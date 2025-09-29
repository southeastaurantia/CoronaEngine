#pragma once

/**
 * Corona 并发工具集主头文件
 * 提供高性能的并发编程工具
 * 
 * 设计目标：
 * - 百万级 ops/s 的吞吐能力
 * - 现代 C++20 特性支持
 * - 跨平台兼容性
 * - 易于使用的 API
 */

// 核心组件
#include "core/atomic.h"   // 原子操作封装
#include "core/thread.h"   // 线程管理和工具
#include "core/memory.h"   // 内存管理
#include "core/sync.h"     // 同步原语

namespace Corona::Concurrent {

// 重新导出核心命名空间中的主要类型
using namespace Core;

/**
 * 版本信息
 */
constexpr struct {
    int major = 1;
    int minor = 0;
    int patch = 0;
    const char* string = "1.0.0";
} version;

/**
 * 初始化并发库
 * 在使用库之前调用，进行必要的全局初始化
 */
void initialize() noexcept;

/**
 * 清理并发库资源
 * 程序结束前调用，清理全局资源
 */
void finalize() noexcept;

/**
 * 获取库的运行时统计信息
 */
struct RuntimeStats {
    std::size_t total_memory_allocated;
    std::size_t total_memory_allocations;
    std::size_t active_threads;
    std::size_t cache_hit_rate_percent;
};

RuntimeStats get_runtime_stats() noexcept;

/**
 * 性能配置选项
 */
struct PerformanceConfig {
    bool enable_thread_local_cache = true;    // 启用线程本地缓存
    bool enable_cpu_affinity = false;         // 启用 CPU 亲和性
    std::size_t slab_pool_size = 1024 * 1024; // Slab 池大小
    std::size_t memory_pool_size = 4 * 1024 * 1024; // 内存池大小
};

/**
 * 设置性能配置
 */
void set_performance_config(const PerformanceConfig& config) noexcept;

/**
 * 获取当前性能配置
 */
PerformanceConfig get_performance_config() noexcept;

} // namespace Corona::Concurrent