#include <compiler_features.h>

#include "../include/core/memory.h"

#include <algorithm>
#include <cstdlib>
#include <new>

#if CE_BUILTIN_PLATFORM_WINDOWS
#include <malloc.h>
#endif

// 跨平台的对齐内存分配函数
inline void* portable_aligned_alloc(std::size_t alignment, std::size_t size) {
#if CE_BUILTIN_PLATFORM_WINDOWS
    return _aligned_malloc(size, alignment);
#elif CE_BUILTIN_PLATFORM_POSIX
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) == 0) {
        return ptr;
    }
    return nullptr;
#else
    if ((size % alignment) != 0) {
        size = ((size + alignment - 1) / alignment) * alignment;
    }
    return std::aligned_alloc(alignment, size);
#endif
}

inline void portable_aligned_free(void* ptr) {
#if CE_BUILTIN_PLATFORM_WINDOWS
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

namespace Corona::Concurrent::Core {

/**
 * CacheLineAllocator 实现
 */
void* CacheLineAllocator::allocate(std::size_t size, std::size_t alignment) {
    // 确保对齐到缓存线边界
    alignment = std::max(alignment, CACHE_LINE_SIZE);
    
    std::size_t aligned_size = align_up(size, alignment);
    
    void* ptr = portable_aligned_alloc(alignment, aligned_size);
    if (!ptr) {
        throw std::bad_alloc();
    }
    
    allocated_bytes_ += aligned_size;
    allocation_count_++;
    
    return ptr;
}

void CacheLineAllocator::deallocate(void* ptr, std::size_t size) noexcept {
    if (!ptr) return;
    
    portable_aligned_free(ptr);
    
    allocated_bytes_ -= size;
}

/**
 * ThreadLocalCache 实现
 */
MemoryChunk* ThreadLocalCache::get_chunk(SizeClass cls) noexcept {
    auto cls_index = static_cast<std::size_t>(cls);
    auto& entry = cache_[cls_index];
    
    if (entry.count > 0) {
        entry.count--;
        MemoryChunk* chunk = entry.chunks[entry.count];
        entry.chunks[entry.count] = nullptr;
        return chunk;
    }
    
    return nullptr;
}

bool ThreadLocalCache::put_chunk(SizeClass cls, MemoryChunk* chunk) noexcept {
    if (!chunk) return false;
    
    auto cls_index = static_cast<std::size_t>(cls);
    auto& entry = cache_[cls_index];
    
    if (entry.count < MAX_CACHED_CHUNKS) {
        entry.chunks[entry.count] = chunk;
        entry.count++;
        return true;
    }
    
    return false; // 缓存已满
}

void ThreadLocalCache::clear() noexcept {
    for (auto& entry : cache_) {
        for (std::size_t i = 0; i < entry.count; ++i) {
            if (entry.chunks[i]) {
                free(entry.chunks[i]->data);
                delete entry.chunks[i];
                entry.chunks[i] = nullptr;
            }
        }
        entry.count = 0;
    }
}

/**
 * SlabAllocator 实现
 */
SlabAllocator::SlabAllocator() {
    // 初始化所有 Slab
    for (std::size_t i = 0; i < static_cast<std::size_t>(SizeClass::COUNT); ++i) {
        init_slab(static_cast<SizeClass>(i));
    }
}

SlabAllocator::~SlabAllocator() {
    // 释放所有 Slab 内存
    for (auto& slab : slabs_) {
        if (slab.memory_base) {
            portable_aligned_free(slab.memory_base);
        }
    }
}

void SlabAllocator::init_slab(SizeClass cls) {
    auto& slab = slabs_[static_cast<std::size_t>(cls)];
    
    slab.chunk_size = get_class_size(cls);
    slab.chunk_count = std::max(static_cast<std::size_t>(64), 4096 / slab.chunk_size); // 至少64个块或4KB
    
    std::size_t total_size = slab.chunk_size * slab.chunk_count;
    slab.memory_base = portable_aligned_alloc(CACHE_LINE_SIZE, total_size);
    
    if (!slab.memory_base) {
        throw std::bad_alloc();
    }
    
    // 构建空闲列表
    MemoryChunk* prev = nullptr;
    for (std::size_t i = 0; i < slab.chunk_count; ++i) {
        auto chunk = new MemoryChunk{
            static_cast<char*>(slab.memory_base) + i * slab.chunk_size,
            slab.chunk_size,
            CACHE_LINE_SIZE,
            prev
        };
        prev = chunk;
    }
    
    slab.free_list.store(prev, std::memory_order_relaxed);
}

MemoryChunk* SlabAllocator::allocate_from_slab(SizeClass cls) {
    auto& slab = slabs_[static_cast<std::size_t>(cls)];
    
    while (true) {
        MemoryChunk* head = slab.free_list.load(std::memory_order_acquire);
        if (!head) {
            return nullptr; // Slab 已满
        }
        
        if (slab.free_list.compare_exchange_weak(head, head->next, 
                                                std::memory_order_release,
                                                std::memory_order_acquire)) {
            slab.used_count++;
            return head;
        }
        // CAS 失败，重试
    }
}

void SlabAllocator::deallocate_to_slab(SizeClass cls, MemoryChunk* chunk) noexcept {
    auto& slab = slabs_[static_cast<std::size_t>(cls)];
    
    while (true) {
        MemoryChunk* head = slab.free_list.load(std::memory_order_acquire);
        chunk->next = head;
        
        if (slab.free_list.compare_exchange_weak(head, chunk,
                                                std::memory_order_release,
                                                std::memory_order_acquire)) {
            slab.used_count--;
            break;
        }
        // CAS 失败，重试
    }
}

void* SlabAllocator::allocate(std::size_t size, std::size_t alignment) {
    SizeClass cls = get_size_class(size);
    
    // 首先尝试从线程本地缓存获取
    auto& tl_cache = AllocatorManager::get_thread_cache();
    MemoryChunk* chunk = tl_cache.get_chunk(cls);
    
    if (!chunk) {
        // 从 Slab 分配
        chunk = allocate_from_slab(cls);
        if (!chunk) {
            // Slab 已满，使用系统分配器
            void* ptr = portable_aligned_alloc(alignment, size);
            if (!ptr) {
                throw std::bad_alloc();
            }
            total_allocated_ += size;
            allocation_count_++;
            return ptr;
        }
    }
    
    total_allocated_ += chunk->size;
    allocation_count_++;
    return chunk->data;
}

void SlabAllocator::deallocate(void* ptr, std::size_t size) noexcept {
    if (!ptr) return;
    
    SizeClass cls = get_size_class(size);
    auto& tl_cache = AllocatorManager::get_thread_cache();
    
    // 创建 chunk 包装器
    auto chunk = new (std::nothrow) MemoryChunk{ptr, size, 0, nullptr};
    if (!chunk) {
        free(ptr); // 内存不足，直接释放
        return;
    }
    
    // 尝试放入线程本地缓存
    if (!tl_cache.put_chunk(cls, chunk)) {
        // 缓存已满，返回给 Slab
        deallocate_to_slab(cls, chunk);
    }
    
    total_allocated_ -= size;
}

double SlabAllocator::slab_usage(SizeClass cls) const noexcept {
    const auto& slab = slabs_[static_cast<std::size_t>(cls)];
    std::size_t used = slab.used_count.load(std::memory_order_relaxed);
    return static_cast<double>(used) / slab.chunk_count;
}

/**
 * MemoryPool 实现
 */
MemoryPool::MemoryPool(std::size_t default_pool_size) 
    : default_pool_size_(default_pool_size) {
}

MemoryPool::~MemoryPool() {
    reset();
}

MemoryPool::Pool* MemoryPool::create_pool(std::size_t min_size) {
    std::size_t pool_size = std::max(min_size, default_pool_size_);
    
    void* memory = portable_aligned_alloc(CACHE_LINE_SIZE, pool_size);
    if (!memory) {
        throw std::bad_alloc();
    }
    
    auto pool = new Pool{
        memory,
        pool_size,
        Atomic<std::size_t>{0},
        nullptr
    };
    
    return pool;
}

void* MemoryPool::allocate_from_pool(std::size_t size, std::size_t alignment) {
    Pool* pool = current_pool_.load(std::memory_order_acquire);
    
    while (pool) {
        std::size_t current_offset = pool->offset.load(std::memory_order_relaxed);
        std::size_t aligned_offset = align_up(current_offset, alignment);
        std::size_t new_offset = aligned_offset + size;
        
        if (new_offset <= pool->pool_size) {
            // 尝试原子性地更新偏移
            if (pool->offset.compare_exchange_weak(current_offset, new_offset,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed)) {
                return static_cast<char*>(pool->memory_base) + aligned_offset;
            }
            // CAS 失败，重试
            continue;
        } else {
            // 当前池空间不足，需要新池
            break;
        }
    }
    
    return nullptr;
}

void* MemoryPool::allocate(std::size_t size, std::size_t alignment) {
    void* ptr = allocate_from_pool(size, alignment);
    if (ptr) {
        total_allocated_ += size;
        allocation_count_++;
        return ptr;
    }
    
    // 需要创建新池
    Pool* new_pool = create_pool(size + alignment);
    
    // 尝试设置为当前池
    Pool* expected = current_pool_.load(std::memory_order_acquire);
    new_pool->next = expected;
    
    while (!current_pool_.compare_exchange_weak(expected, new_pool,
                                               std::memory_order_release,
                                               std::memory_order_acquire)) {
        new_pool->next = expected;
    }
    
    // 从新池分配
    ptr = allocate_from_pool(size, alignment);
    if (!ptr) {
        throw std::bad_alloc(); // 这不应该发生
    }
    
    total_allocated_ += size;
    allocation_count_++;
    return ptr;
}

void MemoryPool::deallocate(void* ptr, std::size_t size) noexcept {
    // 内存池不支持单独释放，只能整体重置
    (void)ptr;
    (void)size;
}

void MemoryPool::reset() {
    Pool* pool = current_pool_.exchange(nullptr, std::memory_order_acq_rel);
    
    while (pool) {
        Pool* next = pool->next;
        free(pool->memory_base);
        delete pool;
        pool = next;
    }
    
    total_allocated_.store(0, std::memory_order_relaxed);
    allocation_count_.store(0, std::memory_order_relaxed);
}

/**
 * AllocatorManager 实现
 */
void* AllocatorManager::allocate(std::size_t size, std::size_t alignment) {
    return slab_allocator_.allocate(size, alignment);
}

void AllocatorManager::deallocate(void* ptr, std::size_t size) noexcept {
    slab_allocator_.deallocate(ptr, size);
}

std::size_t AllocatorManager::total_allocated_bytes() noexcept {
    return slab_allocator_.allocated_bytes() + cache_line_allocator_.allocated_bytes();
}

std::size_t AllocatorManager::total_allocations() noexcept {
    return slab_allocator_.total_allocations() + cache_line_allocator_.total_allocations();
}

} // namespace Corona::Concurrent::Core