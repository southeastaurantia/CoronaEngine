#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "atomic.h"


namespace Corona::Concurrent::Core {

/**
 * 内存对齐工具
 */
constexpr std::size_t align_up(std::size_t size, std::size_t alignment) noexcept {
    return (size + alignment - 1) & ~(alignment - 1);
}

constexpr std::size_t align_down(std::size_t size, std::size_t alignment) noexcept {
    return size & ~(alignment - 1);
}

inline bool is_aligned(const void* ptr, std::size_t alignment) noexcept {
    return (reinterpret_cast<std::uintptr_t>(ptr) & (alignment - 1)) == 0;
}

/**
 * 内存大小类别
 * 用于 Slab 分配器的尺寸分类
 */
enum class SizeClass : std::uint8_t {
    Tiny_8 = 0,   // 8 字节
    Small_16,     // 16 字节
    Small_32,     // 32 字节
    Small_64,     // 64 字节
    Medium_128,   // 128 字节
    Medium_256,   // 256 字节
    Large_512,    // 512 字节
    Large_1024,   // 1024 字节
    XLarge_2048,  // 2048 字节
    XLarge_4096,  // 4096 字节
    COUNT         // 总数量
};

/**
 * 获取指定大小对应的尺寸类别
 */
constexpr SizeClass get_size_class(std::size_t size) noexcept {
    if (size <= 8) return SizeClass::Tiny_8;
    if (size <= 16) return SizeClass::Small_16;
    if (size <= 32) return SizeClass::Small_32;
    if (size <= 64) return SizeClass::Small_64;
    if (size <= 128) return SizeClass::Medium_128;
    if (size <= 256) return SizeClass::Medium_256;
    if (size <= 512) return SizeClass::Large_512;
    if (size <= 1024) return SizeClass::Large_1024;
    if (size <= 2048) return SizeClass::XLarge_2048;
    return SizeClass::XLarge_4096;
}

/**
 * 获取尺寸类别对应的实际大小
 */
constexpr std::size_t get_class_size(SizeClass cls) noexcept {
    constexpr std::array<std::size_t, static_cast<std::size_t>(SizeClass::COUNT)> sizes = {
        8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    return sizes[static_cast<std::size_t>(cls)];
}

/**
 * 自定义分配器接口
 */
class AllocatorInterface {
   public:
    virtual ~AllocatorInterface() = default;

    /**
     * 分配内存
     */
    virtual void* allocate(std::size_t size, std::size_t alignment) = 0;

    /**
     * 释放内存
     */
    virtual void deallocate(void* ptr, std::size_t size) noexcept = 0;

    /**
     * 获取分配器统计信息
     */
    virtual std::size_t allocated_bytes() const noexcept = 0;
    virtual std::size_t total_allocations() const noexcept = 0;
};

/**
 * 缓存线对齐的分配器
 * 确保分配的内存按缓存线对齐，避免伪共享
 */
class CacheLineAllocator : public AllocatorInterface {
   private:
    AtomicSize allocated_bytes_{0};
    AtomicSize allocation_count_{0};

   public:
    void* allocate(std::size_t size, std::size_t alignment) override;
    void deallocate(void* ptr, std::size_t size) noexcept override;

    std::size_t allocated_bytes() const noexcept override {
        return allocated_bytes_.load(std::memory_order_relaxed);
    }

    std::size_t total_allocations() const noexcept override {
        return allocation_count_.load(std::memory_order_relaxed);
    }
};

/**
 * 内存块结构
 * 用于 Slab 分配器的基本单元
 */
struct MemoryChunk {
    void* data;
    std::size_t size;
    std::size_t alignment;
    MemoryChunk* next;  // 链表指针
};

/**
 * 线程本地内存缓存
 * 为每个线程提供独立的内存缓存，减少锁竞争
 */
class ThreadLocalCache {
   private:
    static constexpr std::size_t MAX_CACHED_CHUNKS = 64;

    struct CacheEntry {
        std::array<MemoryChunk*, MAX_CACHED_CHUNKS> chunks{};
        std::size_t count = 0;
    };

    std::array<CacheEntry, static_cast<std::size_t>(SizeClass::COUNT)> cache_;

   public:
    /**
     * 从缓存中获取内存块
     */
    MemoryChunk* get_chunk(SizeClass cls) noexcept;

    /**
     * 将内存块返回到缓存
     */
    bool put_chunk(SizeClass cls, MemoryChunk* chunk) noexcept;

    /**
     * 清空缓存
     */
    void clear() noexcept;

    /**
     * 获取缓存统计信息
     */
    std::size_t cached_count(SizeClass cls) const noexcept {
        return cache_[static_cast<std::size_t>(cls)].count;
    }
};

/**
 * Slab 分配器
 * 提供高效的小对象分配，减少内存碎片
 */
class SlabAllocator : public AllocatorInterface {
   private:
    struct Slab {
        void* memory_base;                  // 内存基址
        std::size_t chunk_size;             // 块大小
        std::size_t chunk_count;            // 块数量
        AtomicPtr<MemoryChunk> free_list;   // 空闲块链表
        Atomic<std::size_t> used_count{0};  // 已使用块数
    };

    std::array<Slab, static_cast<std::size_t>(SizeClass::COUNT)> slabs_;
    AtomicSize total_allocated_{0};
    AtomicSize allocation_count_{0};

    /**
     * 初始化指定尺寸类别的 Slab
     */
    void init_slab(SizeClass cls);

    /**
     * 从 Slab 中分配内存块
     */
    MemoryChunk* allocate_from_slab(SizeClass cls);

    /**
     * 将内存块归还给 Slab
     */
    void deallocate_to_slab(SizeClass cls, MemoryChunk* chunk) noexcept;

   public:
    SlabAllocator();
    ~SlabAllocator();

    void* allocate(std::size_t size, std::size_t alignment) override;
    void deallocate(void* ptr, std::size_t size) noexcept override;

    std::size_t allocated_bytes() const noexcept override {
        return total_allocated_.load(std::memory_order_relaxed);
    }

    std::size_t total_allocations() const noexcept override {
        return allocation_count_.load(std::memory_order_relaxed);
    }

    /**
     * 获取 Slab 使用情况
     */
    double slab_usage(SizeClass cls) const noexcept;
};

/**
 * 内存池分配器
 * 预分配大块内存，减少系统调用开销
 */
class MemoryPool : public AllocatorInterface {
   private:
    struct Pool {
        void* memory_base;
        std::size_t pool_size;
        Atomic<std::size_t> offset{0};  // 当前分配偏移
        Pool* next;
    };

    AtomicPtr<Pool> current_pool_{nullptr};
    AtomicSize total_allocated_{0};
    AtomicSize allocation_count_{0};

    std::size_t default_pool_size_;

    /**
     * 创建新的内存池
     */
    Pool* create_pool(std::size_t min_size);

    /**
     * 从当前池中分配内存
     */
    void* allocate_from_pool(std::size_t size, std::size_t alignment);

   public:
    explicit MemoryPool(std::size_t default_pool_size = 1024 * 1024);  // 默认 1MB
    ~MemoryPool();

    void* allocate(std::size_t size, std::size_t alignment) override;
    void deallocate(void* ptr, std::size_t size) noexcept override;

    std::size_t allocated_bytes() const noexcept override {
        return total_allocated_.load(std::memory_order_relaxed);
    }

    std::size_t total_allocations() const noexcept override {
        return allocation_count_.load(std::memory_order_relaxed);
    }

    /**
     * 重置内存池（释放所有内存）
     */
    void reset();
};

/**
 * 全局分配器管理器
 */
class AllocatorManager {
   private:
    static inline thread_local ThreadLocalCache tl_cache_;
    static inline SlabAllocator slab_allocator_;
    static inline CacheLineAllocator cache_line_allocator_;

   public:
    /**
     * 分配指定大小的内存
     */
    static void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));

    /**
     * 释放内存
     */
    static void deallocate(void* ptr, std::size_t size) noexcept;

    /**
     * 分配缓存线对齐的内存
     */
    static void* allocate_aligned(std::size_t size) {
        return cache_line_allocator_.allocate(size, CACHE_LINE_SIZE);
    }

    /**
     * 获取线程本地缓存
     */
    static ThreadLocalCache& get_thread_cache() noexcept {
        return tl_cache_;
    }

    /**
     * 获取全局统计信息
     */
    static std::size_t total_allocated_bytes() noexcept;
    static std::size_t total_allocations() noexcept;
};

}  // namespace Corona::Concurrent::Core