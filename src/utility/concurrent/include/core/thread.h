#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

#include "atomic.h"


namespace Corona::Concurrent::Core {

/**
 * CPU 核心信息
 */
struct CpuInfo {
    std::uint32_t physical_cores;  // 物理核心数
    std::uint32_t logical_cores;   // 逻辑核心数（包括超线程）
    std::uint32_t numa_nodes;      // NUMA 节点数
    bool has_hyper_threading;      // 是否支持超线程
};

/**
 * 获取 CPU 信息
 */
CpuInfo get_cpu_info() noexcept;

/**
 * 线程 ID 类型
 */
using ThreadId = std::uint64_t;

/**
 * 获取当前线程的唯一 ID
 */
ThreadId get_current_thread_id() noexcept;

/**
 * CPU 亲和性设置
 */
class CpuAffinity {
   public:
    /**
     * 将当前线程绑定到指定 CPU 核心
     * @param cpu_id CPU 核心 ID（0 开始）
     * @return 成功返回 true，失败返回 false
     */
    static bool bind_to_cpu(std::uint32_t cpu_id) noexcept;

    /**
     * 将指定线程绑定到指定 CPU 核心
     */
    static bool bind_thread_to_cpu(std::thread::id thread_id, std::uint32_t cpu_id) noexcept;

    /**
     * 获取当前线程的 CPU 亲和性
     */
    static std::vector<std::uint32_t> get_current_affinity() noexcept;

    /**
     * 重置线程的 CPU 亲和性（允许在所有核心上运行）
     */
    static bool reset_affinity() noexcept;
};

/**
 * 线程本地统计信息
 */
struct ThreadLocalStats {
    std::uint64_t operations_count = 0;  // 操作计数
    std::uint64_t cache_hits = 0;        // 缓存命中次数
    std::uint64_t cache_misses = 0;      // 缓存未命中次数
    std::uint64_t contentions = 0;       // 争用次数

    /**
     * 重置统计信息
     */
    void reset() noexcept {
        operations_count = 0;
        cache_hits = 0;
        cache_misses = 0;
        contentions = 0;
    }

    /**
     * 计算缓存命中率
     */
    double hit_rate() const noexcept {
        auto total = cache_hits + cache_misses;
        return total > 0 ? static_cast<double>(cache_hits) / total : 0.0;
    }
};

/**
 * 线程本地数据管理器
 */
class ThreadLocal {
   public:
    /**
     * 获取当前线程的统计信息
     */
    static ThreadLocalStats& get_stats() noexcept;

    /**
     * 记录一次操作
     */
    static void record_operation() noexcept {
        get_stats().operations_count++;
    }

    /**
     * 记录缓存命中
     */
    static void record_cache_hit() noexcept {
        get_stats().cache_hits++;
    }

    /**
     * 记录缓存未命中
     */
    static void record_cache_miss() noexcept {
        get_stats().cache_misses++;
    }

    /**
     * 记录争用
     */
    static void record_contention() noexcept {
        get_stats().contentions++;
    }
};

/**
 * 高精度时间测量工具
 */
class HighResTimer {
   private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    TimePoint start_time_;

   public:
    /**
     * 开始计时
     */
    void start() noexcept {
        start_time_ = Clock::now();
    }

    /**
     * 获取经过的纳秒数
     */
    std::uint64_t elapsed_nanos() const noexcept {
        auto now = Clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time_);
        return static_cast<std::uint64_t>(duration.count());
    }

    /**
     * 获取经过的微秒数
     */
    std::uint64_t elapsed_micros() const noexcept {
        return elapsed_nanos() / 1000;
    }

    /**
     * 获取经过的毫秒数
     */
    std::uint64_t elapsed_millis() const noexcept {
        return elapsed_nanos() / 1000000;
    }
};

/**
 * 自旋等待工具
 * 提供指数退避策略
 */
class SpinWait {
   private:
    std::uint32_t spin_count_ = 0;
    static constexpr std::uint32_t MAX_SPIN_COUNT = 10;
    static constexpr std::uint32_t YIELD_THRESHOLD = 20;

   public:
    /**
     * 执行一次等待
     * 使用指数退避策略：先自旋，后让出 CPU
     */
    void spin_once() noexcept {
        if (spin_count_ < MAX_SPIN_COUNT) {
            // 短时间自旋
            for (std::uint32_t i = 0; i < (1u << spin_count_); ++i) {
                cpu_relax();
            }
        } else if (spin_count_ < YIELD_THRESHOLD) {
            // 让出 CPU 时间片
            std::this_thread::yield();
        } else {
            // 短暂休眠
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        spin_count_++;
    }

    /**
     * 重置自旋计数
     */
    void reset() noexcept {
        spin_count_ = 0;
    }

    /**
     * 获取当前自旋次数
     */
    std::uint32_t count() const noexcept {
        return spin_count_;
    }
};

/**
 * 线程池前向声明
 */
template <typename Task = std::function<void()>>
class ThreadPool;

}  // namespace Corona::Concurrent::Core