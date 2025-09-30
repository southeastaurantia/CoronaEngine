#include "../include/core/thread.h"

#if defined(_WIN32)
#include <windows.h>
#include <processthreadsapi.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include <sys/sysctl.h>
#include <pthread.h>
#endif

namespace Corona::Concurrent::Core {

/**
 * 获取 CPU 信息的实现
 */
CpuInfo get_cpu_info() noexcept {
    CpuInfo info{};
    
#if defined(_WIN32)
    // Windows 实现
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    info.logical_cores = sys_info.dwNumberOfProcessors;
    
    // 获取物理核心数
    DWORD length = 0;
    GetLogicalProcessorInformation(nullptr, &length);
    if (length > 0) {
        auto buffer = std::make_unique<SYSTEM_LOGICAL_PROCESSOR_INFORMATION[]>(
            length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buffer.get(), &length)) {
            DWORD count = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            for (DWORD i = 0; i < count; ++i) {
                if (buffer[i].Relationship == RelationProcessorCore) {
                    info.physical_cores++;
                }
            }
        }
    }
    
    info.has_hyper_threading = (info.logical_cores > info.physical_cores);
    info.numa_nodes = 1; // 简化处理，实际应该查询 NUMA 信息
    
#elif defined(__linux__)
    // Linux 实现
    info.logical_cores = std::thread::hardware_concurrency();
    info.physical_cores = info.logical_cores; // 简化处理
    info.has_hyper_threading = false;
    info.numa_nodes = 1;
    
#elif defined(__APPLE__)
    // macOS 实现
    size_t size = sizeof(info.logical_cores);
    sysctlbyname("hw.logicalcpu", &info.logical_cores, &size, nullptr, 0);
    sysctlbyname("hw.physicalcpu", &info.physical_cores, &size, nullptr, 0);
    info.has_hyper_threading = (info.logical_cores > info.physical_cores);
    info.numa_nodes = 1;
    
#else
    // 通用实现
    info.logical_cores = std::thread::hardware_concurrency();
    info.physical_cores = info.logical_cores;
    info.has_hyper_threading = false;
    info.numa_nodes = 1;
#endif

    // 防止返回 0
    if (info.logical_cores == 0) info.logical_cores = 1;
    if (info.physical_cores == 0) info.physical_cores = 1;
    
    return info;
}

/**
 * 获取当前线程 ID 的实现
 */
ThreadId get_current_thread_id() noexcept {
    static thread_local ThreadId cached_id = 0;
    if (cached_id == 0) {
        auto id = std::this_thread::get_id();
        cached_id = std::hash<std::thread::id>{}(id);
    }
    return cached_id;
}

/**
 * CPU 亲和性设置实现
 */
bool CpuAffinity::bind_to_cpu(std::uint32_t cpu_id) noexcept {
#if defined(_WIN32)
    DWORD_PTR mask = 1ULL << cpu_id;
    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;
    
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
    
#elif defined(__APPLE__)
    // macOS 不直接支持线程 CPU 亲和性设置
    // 这里返回 true 但实际不执行操作
    (void)cpu_id; // 避免未使用参数警告
    return true;
    
#else
    (void)cpu_id;
    return false;
#endif
}

bool CpuAffinity::bind_thread_to_cpu(std::thread::id thread_id, std::uint32_t cpu_id) noexcept {
#if defined(_WIN32)
    // 这需要线程句柄，标准库的 thread::id 无法直接转换
    // 在实际使用中，应该传递原生线程句柄
    (void)thread_id;
    (void)cpu_id;
    return false;
    
#elif defined(__linux__)
    // 类似问题，需要 pthread_t
    (void)thread_id;
    (void)cpu_id;
    return false;
    
#else
    (void)thread_id;
    (void)cpu_id;
    return false;
#endif
}

std::vector<std::uint32_t> CpuAffinity::get_current_affinity() noexcept {
    std::vector<std::uint32_t> result;
    
#if defined(_WIN32)
    DWORD_PTR process_mask, system_mask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
        for (std::uint32_t i = 0; i < 64; ++i) {
            if (process_mask & (1ULL << i)) {
                result.push_back(i);
            }
        }
    }
    
#elif defined(__linux__)
    cpu_set_t cpuset;
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0) {
        for (std::uint32_t i = 0; i < CPU_SETSIZE; ++i) {
            if (CPU_ISSET(i, &cpuset)) {
                result.push_back(i);
            }
        }
    }
    
#else
    // 返回所有可用核心
    auto cpu_info = get_cpu_info();
    for (std::uint32_t i = 0; i < cpu_info.logical_cores; ++i) {
        result.push_back(i);
    }
#endif
    
    return result;
}

bool CpuAffinity::reset_affinity() noexcept {
#if defined(_WIN32)
    DWORD_PTR process_mask, system_mask;
    if (GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
        return SetThreadAffinityMask(GetCurrentThread(), process_mask) != 0;
    }
    return false;
    
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        CPU_SET(i, &cpuset);
    }
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0;
    
#else
    return true; // 假设成功
#endif
}

/**
 * 线程本地统计信息实现
 */
ThreadLocalStats& ThreadLocal::get_stats() noexcept {
    static thread_local ThreadLocalStats stats;
    return stats;
}

} // namespace Corona::Concurrent::Core