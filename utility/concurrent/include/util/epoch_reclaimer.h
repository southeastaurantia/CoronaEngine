#pragma once

#include "../core/atomic.h"
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <algorithm>
#include <thread>
#include <stdexcept>
#include <limits>
#include <atomic>

namespace Corona::Concurrent {

/**
 * Epoch-based 内存回收器
 * 
 * 基于 RCU (Read-Copy-Update) 思想的无锁内存回收机制
 * 特性：
 * - 高吞吐量：读操作几乎无开销
 * - 线程安全：自动处理内存回收时机
 * - ABA安全：防止野指针访问
 * - 批量回收：提高回收效率
 */
template<typename T>
class EpochReclaimer {
private:
    // 全局epoch计数器
    static inline Core::AtomicSize global_epoch_{0};

    static constexpr std::size_t BASE_CLEANUP_THRESHOLD = 64;
    static constexpr std::size_t MIN_CLEANUP_THRESHOLD = 8;
    static constexpr std::size_t CLEANUP_SCALE = 2;
    
    // 每个线程的epoch记录
    struct alignas(Core::CACHE_LINE_SIZE) ThreadEpoch {
        Core::AtomicSize local_epoch{0};
        std::atomic<bool> active{false};
        std::atomic<bool> registered{false};
        
        ThreadEpoch() = default;
        ThreadEpoch(const ThreadEpoch&) = delete;
        ThreadEpoch& operator=(const ThreadEpoch&) = delete;
    };
    
    // 待回收对象
    struct RetiredObject {
        T* ptr;
        std::size_t epoch;
        std::function<void(T*)> deleter;
        
        RetiredObject(T* p, std::size_t e, std::function<void(T*)> d = nullptr)
            : ptr(p), epoch(e), deleter(d ? std::move(d) : [](T* obj) { delete obj; }) {}
    };
    
    // 线程本地数据
    struct alignas(Core::CACHE_LINE_SIZE) ThreadLocalData {
        std::vector<RetiredObject> retired_objects;
        ThreadEpoch* epoch_record;
        std::size_t epoch_index;
        std::size_t last_cleanup_epoch;
        
        ThreadLocalData() : epoch_record(nullptr), epoch_index(MAX_THREADS), last_cleanup_epoch(0) {
            retired_objects.reserve(BASE_CLEANUP_THRESHOLD * 2);  // 预分配空间
        }
        
        ~ThreadLocalData() {
            // 析构时强制清理所有对象
            force_cleanup();
            if (epoch_record) {
                epoch_record->active.store(false, std::memory_order_release);
                epoch_record->registered.store(false, std::memory_order_release);
                epoch_record->local_epoch.store(0, std::memory_order_relaxed);
                registered_records_.fetch_sub(1, std::memory_order_acq_rel);
                release_index(epoch_index);
                epoch_record = nullptr;
                epoch_index = MAX_THREADS;
            }
        }
        
        void force_cleanup() {
            for (const auto& obj : retired_objects) {
                obj.deleter(obj.ptr);
            }
            retired_objects.clear();
        }
    };
    
    // 全局线程epoch记录数组
    static constexpr std::size_t MAX_THREADS = 128;
    static inline std::array<ThreadEpoch, MAX_THREADS> thread_epochs_;
    static inline std::array<std::atomic<bool>, MAX_THREADS> index_slots_{};
    static inline std::atomic_size_t registered_records_{0};
    
    // 线程本地数据
    static thread_local ThreadLocalData tl_data_;
    
    // 清理参数
    static constexpr std::size_t EPOCH_ADVANCE_THRESHOLD = 32;  // epoch推进阈值

public:
    /**
     * RAII 保护类，用于标记临界区
     */
    class Guard {
    private:
        ThreadEpoch* epoch_record_;
        std::size_t saved_epoch_;
        
    public:
        explicit Guard([[maybe_unused]] const EpochReclaimer& reclaimer) {
            // 获取线程的epoch记录
            if (!tl_data_.epoch_record) {
                acquire_epoch_record();
            }
            
            epoch_record_ = tl_data_.epoch_record;
            if (epoch_record_) {
                // 进入临界区，更新本地epoch
                saved_epoch_ = global_epoch_.load(std::memory_order_acquire);
                epoch_record_->local_epoch.store(saved_epoch_, std::memory_order_release);
                epoch_record_->active.store(true, std::memory_order_release);
            }
        }
        
        ~Guard() {
            if (epoch_record_) {
                // 离开临界区
                epoch_record_->active.store(false, std::memory_order_release);
                
                // 定期推进全局epoch
                static thread_local std::size_t operation_count = 0;
                if (++operation_count >= EPOCH_ADVANCE_THRESHOLD) {
                    operation_count = 0;
                    try_advance_global_epoch();
                }
            }
        }
        
        // 禁用拷贝和移动
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&&) = delete;
        Guard& operator=(Guard&&) = delete;
        
    private:
        void acquire_epoch_record() {
            constexpr std::size_t kMaxAttempts = 1024;

            for (std::size_t attempt = 0; attempt < kMaxAttempts; ++attempt) {
                std::size_t index = acquire_index();
                if (index < MAX_THREADS) {
                    auto& record = thread_epochs_[index];
                    bool expected = false;
                    if (record.registered.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                        record.active.store(false, std::memory_order_relaxed);
                        record.local_epoch.store(0, std::memory_order_relaxed);
                        tl_data_.epoch_record = &record;
                        tl_data_.epoch_index = index;
                        registered_records_.fetch_add(1, std::memory_order_acq_rel);
                        return;
                    }
                    release_index(index);
                }
                std::this_thread::yield();
            }

            throw std::runtime_error("EpochReclaimer: exhausted thread epoch slots; increase MAX_THREADS");
        }
        
        void try_advance_global_epoch() {
            std::size_t current_epoch = global_epoch_.load(std::memory_order_relaxed);
            
            // 检查是否所有活跃线程都推进到当前epoch
            std::size_t min_epoch = current_epoch;
            bool has_active_threads = false;
            
            const std::size_t registered = registered_records_.load(std::memory_order_acquire);
            if (registered == 0) {
                global_epoch_.compare_exchange_strong(current_epoch, current_epoch + 1,
                                                    std::memory_order_acq_rel);
                return;
            }

            for (const auto& record : thread_epochs_) {
                if (!record.registered.load(std::memory_order_acquire)) {
                    continue;
                }
                if (record.active.load(std::memory_order_acquire)) {
                    has_active_threads = true;
                    std::size_t local_epoch = record.local_epoch.load(std::memory_order_acquire);
                    min_epoch = std::min(min_epoch, local_epoch);
                }
            }
            
            // 如果所有活跃线程都到达当前epoch，推进全局epoch
            if (!has_active_threads || min_epoch >= current_epoch) {
                global_epoch_.compare_exchange_strong(current_epoch, current_epoch + 1,
                                                    std::memory_order_acq_rel);
            }
        }
    };
    
    /**
     * 将对象标记为退休，等待安全时机回收
     * @param ptr 要回收的对象指针
     * @param deleter 自定义删除器，默认使用 delete
     */
    void retire(T* ptr, std::function<void(T*)> deleter = nullptr) {
        if (!ptr) return;
        
        // 确保线程本地数据已初始化
        if (!tl_data_.epoch_record) {
            Guard dummy_guard(*this);  // 初始化epoch记录
        }
        
        std::size_t current_epoch = global_epoch_.load(std::memory_order_acquire);
        tl_data_.retired_objects.emplace_back(ptr, current_epoch, std::move(deleter));
        
        // 检查是否需要清理
        const std::size_t registered = registered_records_.load(std::memory_order_acquire);
        const std::size_t threshold = compute_cleanup_threshold(registered);
        if (tl_data_.retired_objects.size() >= threshold) {
            try_cleanup();
        }
    }
    
    /**
     * 立即尝试清理可以安全回收的对象
     */
    void try_cleanup() {
        std::size_t safe_epoch = get_safe_epoch();
        
        // 清理epoch小于安全epoch的对象
        auto it = tl_data_.retired_objects.begin();
        while (it != tl_data_.retired_objects.end()) {
            if (it->epoch < safe_epoch) {
                it->deleter(it->ptr);
                it = tl_data_.retired_objects.erase(it);
            } else {
                ++it;
            }
        }
        
        tl_data_.last_cleanup_epoch = safe_epoch;
    }
    
    /**
     * 强制清理所有待回收对象（用于程序退出）
     */
    void force_cleanup_all() {
        tl_data_.force_cleanup();
    }
    
    /**
     * 获取当前全局epoch
     */
    static std::size_t current_epoch() {
        return global_epoch_.load(std::memory_order_acquire);
    }
    
    /**
     * 获取统计信息
     */
    struct Statistics {
        std::size_t pending_objects;
        std::size_t global_epoch;
        std::size_t safe_epoch;
        std::size_t active_threads;
        std::size_t registered_threads;
    };
    
    Statistics get_statistics() const {
        Statistics stats;
        stats.pending_objects = tl_data_.retired_objects.size();
        stats.global_epoch = global_epoch_.load(std::memory_order_relaxed);
        stats.safe_epoch = get_safe_epoch();
        
        stats.active_threads = 0;
        stats.registered_threads = registered_records_.load(std::memory_order_acquire);
        for (const auto& record : thread_epochs_) {
            if (!record.registered.load(std::memory_order_relaxed)) {
                continue;
            }
            if (record.active.load(std::memory_order_relaxed)) {
                stats.active_threads++;
            }
        }
        
        return stats;
    }

private:
    static std::size_t acquire_index() {
        for (std::size_t i = 0; i < MAX_THREADS; ++i) {
            bool expected = false;
            if (index_slots_[i].compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                return i;
            }
        }
        return MAX_THREADS;
    }

    static void release_index(std::size_t index) {
        if (index < MAX_THREADS) {
            index_slots_[index].store(false, std::memory_order_release);
        }
    }

    static std::size_t compute_cleanup_threshold(std::size_t registered) {
        if (registered == 0) {
            return MIN_CLEANUP_THRESHOLD;
        }
        std::size_t scaled = registered * CLEANUP_SCALE;
        if (scaled < MIN_CLEANUP_THRESHOLD) {
            scaled = MIN_CLEANUP_THRESHOLD;
        } else if (scaled > BASE_CLEANUP_THRESHOLD) {
            scaled = BASE_CLEANUP_THRESHOLD;
        }
        return scaled;
    }

    /**
     * 计算安全的回收epoch
     * 只有epoch小于此值的对象才能被安全回收
     */
    std::size_t get_safe_epoch() const {
        std::size_t min_epoch = global_epoch_.load(std::memory_order_acquire);
        bool found_active = false;
        
        for (const auto& record : thread_epochs_) {
            if (!record.registered.load(std::memory_order_acquire)) {
                continue;
            }
            if (record.active.load(std::memory_order_acquire)) {
                found_active = true;
                std::size_t local_epoch = record.local_epoch.load(std::memory_order_acquire);
                min_epoch = std::min(min_epoch, local_epoch);
            }
        }
        
        // 如果没有活跃线程，可以回收所有对象
        // 否则返回最小的活跃epoch
        if (!found_active) {
            const std::size_t registered = registered_records_.load(std::memory_order_acquire);
            return registered == 0 ? std::numeric_limits<std::size_t>::max() : min_epoch + 1;
        }
        return min_epoch;
    }
};

// 静态成员定义
template<typename T>
thread_local typename EpochReclaimer<T>::ThreadLocalData EpochReclaimer<T>::tl_data_;

/**
 * 通用内存回收器 - 自动选择最佳策略
 */
class MemoryReclaimer {
private:
    // 使用不同策略的标志
    enum class Strategy {
        IMMEDIATE,      // 立即删除（调试模式）
        EPOCH_BASED,    // Epoch-based回收
        HAZARD_POINTER  // Hazard Pointer回收
    };
    
    static constexpr Strategy current_strategy_ = Strategy::EPOCH_BASED;
    
public:
    class Guard {
    private:
        std::unique_ptr<void, std::function<void(void*)>> guard_impl_;
        
    public:
        template<typename T>
        explicit Guard(const EpochReclaimer<T>& reclaimer) {
            if constexpr (current_strategy_ == Strategy::EPOCH_BASED) {
                auto* epoch_guard = new typename EpochReclaimer<T>::Guard(reclaimer);
                guard_impl_ = std::unique_ptr<void, std::function<void(void*)>>(
                    epoch_guard,
                    [](void* ptr) { delete static_cast<typename EpochReclaimer<T>::Guard*>(ptr); }
                );
            }
        }
        
        explicit Guard(const MemoryReclaimer&) {
            // 通用构造函数，使用默认策略
        }
        
        ~Guard() = default;
        
        // 禁用拷贝和移动
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;
        Guard(Guard&&) = default;
        Guard& operator=(Guard&&) = default;
    };
    
    template<typename T>
    void retire(T* ptr) {
        if constexpr (current_strategy_ == Strategy::IMMEDIATE) {
            delete ptr;
        } else if constexpr (current_strategy_ == Strategy::EPOCH_BASED) {
            static thread_local EpochReclaimer<T> epoch_reclaimer;
            epoch_reclaimer.retire(ptr);
        }
    }
    
    template<typename T>
    void retire(T* ptr, std::function<void(T*)> deleter) {
        if constexpr (current_strategy_ == Strategy::IMMEDIATE) {
            deleter(ptr);
        } else if constexpr (current_strategy_ == Strategy::EPOCH_BASED) {
            static thread_local EpochReclaimer<T> epoch_reclaimer;
            epoch_reclaimer.retire(ptr, std::move(deleter));
        }
    }
};

} // namespace Corona::Concurrent