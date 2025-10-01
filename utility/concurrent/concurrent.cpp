#include "include/concurrent.h"

namespace Corona::Concurrent {

namespace {
    // 全局配置
    PerformanceConfig g_config{};
    
    // 初始化标志
    std::atomic<bool> g_initialized{false};
}

void initialize() noexcept {
    bool expected = false;
    if (!g_initialized.compare_exchange_strong(expected, true)) {
        return; // 已经初始化
    }
    
    // 执行全局初始化
    // 获取 CPU 信息并进行优化配置
    auto cpu_info = Core::get_cpu_info();
    
    // 如果启用了 CPU 亲和性，可以在这里设置
    if (g_config.enable_cpu_affinity && cpu_info.physical_cores > 1) {
        // 为主线程设置亲和性（可选）
        Core::CpuAffinity::bind_to_cpu(0);
    }
}

void finalize() noexcept {
    bool expected = true;
    if (!g_initialized.compare_exchange_strong(expected, false)) {
        return; // 未初始化或已清理
    }
    
    // 清理全局资源
    // 线程本地缓存会在线程结束时自动清理
}

RuntimeStats get_runtime_stats() noexcept {
    RuntimeStats stats{};
    
    // 收集内存统计
    stats.total_memory_allocated = Core::AllocatorManager::total_allocated_bytes();
    stats.total_memory_allocations = Core::AllocatorManager::total_allocations();
    
    // 获取当前线程的统计信息
    auto& thread_stats = Core::ThreadLocal::get_stats();
    auto total_ops = thread_stats.cache_hits + thread_stats.cache_misses;
    stats.cache_hit_rate_percent = total_ops > 0 ? 
        (thread_stats.cache_hits * 100) / total_ops : 0;
    
    // 活跃线程数（简化实现）
    stats.active_threads = 1; // 当前只统计主线程
    
    return stats;
}

void set_performance_config(const PerformanceConfig& config) noexcept {
    g_config = config;
}

PerformanceConfig get_performance_config() noexcept {
    return g_config;
}

} // namespace Corona::Concurrent