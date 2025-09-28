#pragma once

#include <cstddef>
#include <new>

namespace Corona::Concurrent::Detail {

/// \brief 返回当前平台建议的缓存行大小，用于规避伪共享。
constexpr std::size_t CacheLineSize() {
#if defined(__cpp_lib_hardware_interference_size)
    return std::hardware_destructive_interference_size;
#else
    return 64;
#endif
}

/// \brief 根据元素大小动态计算单个页面可容纳的槽位数量。
template <typename T>
constexpr std::size_t ItemsPerPage() {
    if constexpr (sizeof(T) <= 8) {
        return 32;
    } else if constexpr (sizeof(T) <= 16) {
        return 16;
    } else if constexpr (sizeof(T) <= 32) {
        return 8;
    } else if constexpr (sizeof(T) <= 64) {
        return 4;
    } else if constexpr (sizeof(T) <= 128) {
        return 2;
    } else {
        return 1;
    }
}

} // namespace Corona::Concurrent::Detail
