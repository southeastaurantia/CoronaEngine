#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <functional>
#include <stdexcept>
#include <thread>
#include <vector>

#include "../core/atomic.h"
#include "../detail/runtime_init.h"


namespace Corona::Concurrent {

/**
 * Hazard Pointer 内存回收系统
 *
 * 专门为无锁数据结构设计的内存管理系统
 * 特性：
 * - 无锁设计：读操作无阻塞
 * - 延迟回收：安全时机才释放内存
 * - 线程安全：多线程环境下的安全内存管理
 * - 高效扫描：批量检测和回收
 */
template <typename T>
class HazardPointer {
   public:
    static constexpr std::size_t MAX_THREADS = 128;
    static constexpr std::size_t HAZARDS_PER_THREAD = 4;      // 每线程hazard指针数量
    static constexpr std::size_t BASE_RETIRE_THRESHOLD = 64;  // 默认退休列表阈值
    static constexpr std::size_t MIN_RETIRE_THRESHOLD = 8;    // 空闲态最小阈值
    static constexpr std::size_t RETIRE_SCALE = 2;            // 每注册线程的增量

   private:
    // Hazard 记录结构
    struct alignas(Core::CACHE_LINE_SIZE) HazardRecord {
        std::array<Core::Atomic<T*>, HAZARDS_PER_THREAD> hazards;
        Core::Atomic<std::thread::id> owner;
        std::atomic<bool> active{false};

        HazardRecord() {
            for (auto& hazard : hazards) {
                hazard.store(nullptr, std::memory_order_relaxed);
            }
            owner.store(std::thread::id{}, std::memory_order_relaxed);
        }

        // 禁用拷贝构造和赋值
        HazardRecord(const HazardRecord&) = delete;
        HazardRecord& operator=(const HazardRecord&) = delete;
    };

    // 退休对象
    struct RetiredPointer {
        T* ptr;
        std::function<void(T*)> deleter;

        explicit RetiredPointer(T* p, std::function<void(T*)> d = nullptr)
            : ptr(p), deleter(d ? std::move(d) : [](T* obj) { delete obj; }) {}
    };

    // 线程本地数据
    struct ThreadLocalData {
        HazardRecord* my_hazard_record;
        std::vector<RetiredPointer> retired_list;
        std::vector<T*> hazard_snapshot;

        ThreadLocalData() : my_hazard_record(nullptr) {
            // 移除 reserve 调用,避免在 TLS 初始化时触发 CRT 锁
            // 这可以防止在程序启动早期阶段发生访问违例
            // Vector 会在需要时自动增长,性能影响可忽略不计
        }

        // 延迟初始化容量,在首次使用时调用
        void ensure_capacity() {
            if (retired_list.capacity() < BASE_RETIRE_THRESHOLD * 2) {
                retired_list.reserve(BASE_RETIRE_THRESHOLD * 2);
            }
            if (hazard_snapshot.capacity() < BASE_RETIRE_THRESHOLD * HAZARDS_PER_THREAD) {
                hazard_snapshot.reserve(BASE_RETIRE_THRESHOLD * HAZARDS_PER_THREAD);
            }
        }

        ~ThreadLocalData() {
            // 析构时释放所有退休对象
            for (const auto& retired : retired_list) {
                retired.deleter(retired.ptr);
            }
            retired_list.clear();
            hazard_snapshot.clear();

            // 释放hazard记录
            if (my_hazard_record) {
                my_hazard_record->active.store(false, std::memory_order_release);
                my_hazard_record->owner.store(std::thread::id{}, std::memory_order_release);
                for (auto& hazard : my_hazard_record->hazards) {
                    hazard.store(nullptr, std::memory_order_release);
                }
                registered_records_.fetch_sub(1, std::memory_order_acq_rel);
                my_hazard_record = nullptr;
            }
        }
    };

    // 全局hazard记录数组
    static inline std::array<HazardRecord, MAX_THREADS> hazard_records_;
    static inline std::atomic_size_t registered_records_{0};

    // 线程本地数据
    static thread_local ThreadLocalData tl_data_;

   public:
    /**
     * Hazard Pointer 保护类
     */
    class Guard {
       private:
        HazardRecord* record_;
        std::size_t slot_;

       public:
        explicit Guard(std::size_t slot = 0) : record_(nullptr), slot_(slot) {
            detail::ensure_runtime_initialized();
            if (slot >= HAZARDS_PER_THREAD) {
                slot_ = 0;  // 防止越界
            }

            // 获取当前线程的hazard记录
            if (!tl_data_.my_hazard_record) {
                acquire_hazard_record();
            }
            record_ = tl_data_.my_hazard_record;
        }

        /**
         * 保护指针，防止其被回收
         * @param ptr 要保护的指针
         */
        void protect(T* ptr) {
            if (record_) {
                record_->hazards[slot_].store(ptr, std::memory_order_release);
            }
        }

        /**
         * 原子性地加载并保护指针
         * @param atomic_ptr 原子指针的引用
         * @return 加载的指针值
         */
        T* protect_and_load(const Core::Atomic<T*>& atomic_ptr) {
            T* ptr = nullptr;
            if (record_) {
                do {
                    ptr = atomic_ptr.load(std::memory_order_acquire);
                    record_->hazards[slot_].store(ptr, std::memory_order_release);
                    // 双重检查，确保指针没有在保护期间被修改
                } while (ptr != atomic_ptr.load(std::memory_order_acquire));
            }
            return ptr;
        }

        /**
         * 清除保护
         */
        void reset() {
            if (record_) {
                record_->hazards[slot_].store(nullptr, std::memory_order_release);
            }
        }

        /**
         * 获取当前保护的指针
         */
        T* get_protected() const {
            if (record_) {
                return record_->hazards[slot_].load(std::memory_order_acquire);
            }
            return nullptr;
        }

        ~Guard() {
            reset();
        }

        // 禁用拷贝，允许移动
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        Guard(Guard&& other) noexcept : record_(other.record_), slot_(other.slot_) {
            other.record_ = nullptr;
        }

        Guard& operator=(Guard&& other) noexcept {
            if (this != &other) {
                reset();
                record_ = other.record_;
                slot_ = other.slot_;
                other.record_ = nullptr;
            }
            return *this;
        }

       private:
        void acquire_hazard_record() {
            std::thread::id current_thread_id = std::this_thread::get_id();

            // 首先尝试找到已经属于当前线程的记录
            for (auto& record : hazard_records_) {
                if (record.owner.load(std::memory_order_acquire) == current_thread_id) {
                    record.active.store(true, std::memory_order_release);
                    tl_data_.my_hazard_record = &record;
                    // 在重新激活已有记录时也确保容量
                    tl_data_.ensure_capacity();
                    return;
                }
            }

            // 如果没找到，尝试获取一个未使用的记录
            constexpr std::size_t kMaxAttempts = 1024;
            for (std::size_t attempt = 0; attempt < kMaxAttempts; ++attempt) {
                for (auto& record : hazard_records_) {
                    std::thread::id expected{};
                    if (record.owner.compare_exchange_strong(expected, current_thread_id,
                                                             std::memory_order_acq_rel)) {
                        record.active.store(true, std::memory_order_release);
                        registered_records_.fetch_add(1, std::memory_order_acq_rel);
                        tl_data_.my_hazard_record = &record;
                        // 在获取 hazard 记录后确保容量
                        tl_data_.ensure_capacity();
                        return;
                    }
                }
                std::this_thread::yield();
            }

            throw std::runtime_error("HazardPointer: exhausted hazard records; consider increasing MAX_THREADS");
        }
    };

    /**
     * 将对象标记为退休，等待安全回收
     * @param ptr 要回收的对象指针
     * @param deleter 自定义删除器
     */
    static void retire(T* ptr, std::function<void(T*)> deleter = nullptr) {
        if (!ptr) return;

        auto registered = registered_records_.load(std::memory_order_acquire);
        if (registered == 0) {
            if (deleter) {
                deleter(ptr);
            } else {
                delete ptr;
            }
            return;
        }

        // 确保线程本地数据已初始化
        if (!tl_data_.my_hazard_record) {
            Guard dummy_guard;  // 这会初始化hazard记录
            registered = registered_records_.load(std::memory_order_acquire);
            if (registered == 0) {
                if (deleter) {
                    deleter(ptr);
                } else {
                    delete ptr;
                }
                return;
            }
        }

        // 确保容量已分配
        tl_data_.ensure_capacity();

        tl_data_.retired_list.emplace_back(ptr, std::move(deleter));

        // 检查是否需要执行扫描回收
        const std::size_t threshold = compute_retire_threshold(registered);
        if (tl_data_.retired_list.size() >= threshold) {
            scan_and_reclaim();
        }
    }

    /**
     * 扫描并回收安全的对象
     */
    static void scan_and_reclaim() {
        // 确保容量已分配
        tl_data_.ensure_capacity();

        auto& retired_list = tl_data_.retired_list;
        const std::size_t registered = registered_records_.load(std::memory_order_acquire);

        if (registered == 0) {
            for (const auto& retired : retired_list) {
                retired.deleter(retired.ptr);
            }
            retired_list.clear();
            return;
        }

        // 收集所有当前受保护的指针
        auto& hazard_pointers = tl_data_.hazard_snapshot;
        hazard_pointers.clear();
        const std::size_t desired_capacity = registered * HAZARDS_PER_THREAD;
        if (hazard_pointers.capacity() < desired_capacity) {
            hazard_pointers.reserve(desired_capacity);
        }

        for (const auto& record : hazard_records_) {
            if (record.active.load(std::memory_order_acquire)) {
                for (const auto& hazard : record.hazards) {
                    T* ptr = hazard.load(std::memory_order_acquire);
                    if (ptr) {
                        hazard_pointers.push_back(ptr);
                    }
                }
            }
        }

        // 排序hazard指针以便于二分查找
        std::sort(hazard_pointers.begin(), hazard_pointers.end());

        // 扫描退休列表，回收安全的对象
        auto it = retired_list.begin();

        while (it != retired_list.end()) {
            // 检查这个指针是否在hazard列表中
            bool is_hazardous = std::binary_search(hazard_pointers.begin(),
                                                   hazard_pointers.end(),
                                                   it->ptr);

            if (!is_hazardous) {
                // 安全回收
                it->deleter(it->ptr);
                it = retired_list.erase(it);
            } else {
                ++it;
            }
        }

        hazard_pointers.clear();
    }

    /**
     * 强制清理所有退休对象（程序退出时使用）
     */
    static void force_cleanup_all() {
        for (const auto& retired : tl_data_.retired_list) {
            retired.deleter(retired.ptr);
        }
        tl_data_.retired_list.clear();
    }

    /**
     * 获取统计信息
     */
    struct Statistics {
        std::size_t active_threads;
        std::size_t protected_pointers;
        std::size_t retired_objects;
        std::size_t total_hazard_records;
        std::size_t registered_records;
    };

    static Statistics get_statistics() {
        Statistics stats{};
        stats.total_hazard_records = MAX_THREADS;
        stats.retired_objects = tl_data_.retired_list.size();
        stats.registered_records = registered_records_.load(std::memory_order_acquire);

        for (const auto& record : hazard_records_) {
            if (record.active.load(std::memory_order_acquire)) {
                stats.active_threads++;
                for (const auto& hazard : record.hazards) {
                    if (hazard.load(std::memory_order_acquire)) {
                        stats.protected_pointers++;
                    }
                }
            }
        }

        return stats;
    }

    /**
     * 执行维护操作（定期调用）
     */
    static void maintenance() {
        scan_and_reclaim();
    }

   private:
    static std::size_t compute_retire_threshold(std::size_t registered) {
        std::size_t scaled = registered * RETIRE_SCALE;
        if (scaled < MIN_RETIRE_THRESHOLD) {
            scaled = MIN_RETIRE_THRESHOLD;
        }
        if (scaled > BASE_RETIRE_THRESHOLD) {
            scaled = BASE_RETIRE_THRESHOLD;
        }
        return scaled;
    }
};

// 静态成员定义
template <typename T>
thread_local typename HazardPointer<T>::ThreadLocalData HazardPointer<T>::tl_data_;

}  // namespace Corona::Concurrent