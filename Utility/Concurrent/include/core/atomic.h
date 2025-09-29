#pragma once

#include <atomic>
#include <type_traits>
#include <cstdint>
#include <thread>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#include <intrin.h>
#endif

namespace Corona::Concurrent::Core {

/**
 * 缓存线对齐常量
 * 现代 CPU 通常使用 64 字节缓存线
 */
constexpr size_t CACHE_LINE_SIZE = 64;

/**
 * 缓存线对齐的包装器，防止伪共享
 */
template<typename T>
struct alignas(CACHE_LINE_SIZE) CacheLineAligned {
    T value;
    
    template<typename... Args>
    explicit CacheLineAligned(Args&&... args) 
        : value(std::forward<Args>(args)...) {}
    
    T& get() noexcept { return value; }
    const T& get() const noexcept { return value; }
    
    explicit operator T&() noexcept { return value; }
    explicit operator const T&() const noexcept { return value; }
};

/**
 * 原子操作的统一接口
 * 提供类型安全的原子操作封装
 */
template<typename T>
class Atomic {
private:
    mutable std::atomic<T> value_;

public:
    using value_type = T;
    
    constexpr Atomic() noexcept : value_{} {}
    explicit constexpr Atomic(T initial_value) noexcept : value_(initial_value) {}
    
    // 禁止拷贝和移动
    Atomic(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) = delete;
    Atomic(Atomic&&) = delete;
    Atomic& operator=(Atomic&&) = delete;
    
    /**
     * 原子加载操作
     */
    T load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return value_.load(order);
    }
    
    /**
     * 原子存储操作
     */
    void store(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        value_.store(desired, order);
    }
    
    /**
     * 原子交换操作
     */
    T exchange(T desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.exchange(desired, order);
    }
    
    /**
     * 比较并交换（弱版本）
     */
    bool compare_exchange_weak(T& expected, T desired,
                              std::memory_order success = std::memory_order_seq_cst,
                              std::memory_order failure = std::memory_order_seq_cst) noexcept {
        return value_.compare_exchange_weak(expected, desired, success, failure);
    }
    
    /**
     * 比较并交换（强版本）
     */
    bool compare_exchange_strong(T& expected, T desired,
                                std::memory_order success = std::memory_order_seq_cst,
                                std::memory_order failure = std::memory_order_seq_cst) noexcept {
        return value_.compare_exchange_strong(expected, desired, success, failure);
    }
    
    /**
     * 获取底层原子对象的引用
     */
    std::atomic<T>& native() noexcept { return value_; }
    const std::atomic<T>& native() const noexcept { return value_; }
    
    // 整数类型特有的操作
    template<typename U = T>
    requires std::is_integral_v<U>
    U fetch_add(U arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_add(arg, order);
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U fetch_sub(U arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_sub(arg, order);
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U fetch_and(U arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_and(arg, order);
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U fetch_or(U arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_or(arg, order);
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U fetch_xor(U arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_xor(arg, order);
    }
    
    // 指针类型特有的操作
    template<typename U = T>
    requires std::is_pointer_v<U>
    U fetch_add(std::ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_add(arg, order);
    }
    
    template<typename U = T>
    requires std::is_pointer_v<U>
    U fetch_sub(std::ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.fetch_sub(arg, order);
    }
    
    // 便利操作符（仅对整数类型）
    template<typename U = T>
    requires std::is_integral_v<U>
    U operator++() noexcept {
        return fetch_add(static_cast<U>(1)) + 1;
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U operator++(int) noexcept {
        return fetch_add(static_cast<U>(1));
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U operator--() noexcept {
        return fetch_sub(static_cast<U>(1)) - 1;
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U operator--(int) noexcept {
        return fetch_sub(static_cast<U>(1));
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U operator+=(U arg) noexcept {
        return fetch_add(arg) + arg;
    }
    
    template<typename U = T>
    requires std::is_integral_v<U>
    U operator-=(U arg) noexcept {
        return fetch_sub(arg) - arg;
    }
};

// 类型别名，便于使用
using AtomicBool = Atomic<bool>;
using AtomicInt8 = Atomic<std::int8_t>;
using AtomicUInt8 = Atomic<std::uint8_t>;
using AtomicInt16 = Atomic<std::int16_t>;
using AtomicUInt16 = Atomic<std::uint16_t>;
using AtomicInt32 = Atomic<std::int32_t>;
using AtomicUInt32 = Atomic<std::uint32_t>;
using AtomicInt64 = Atomic<std::int64_t>;
using AtomicUInt64 = Atomic<std::uint64_t>;
using AtomicSize = Atomic<std::size_t>;
using AtomicPtrDiff = Atomic<std::ptrdiff_t>;

template<typename T>
using AtomicPtr = Atomic<T*>;

/**
 * 双宽 CAS 支持结构（128位）
 * 用于实现无锁数据结构中的 ABA 问题解决方案
 */
struct DoubleWord {
    alignas(16) std::uint64_t low;
    std::uint64_t high;
    
    constexpr DoubleWord() noexcept : low(0), high(0) {}
    constexpr DoubleWord(std::uint64_t l, std::uint64_t h) noexcept : low(l), high(h) {}
    
    bool operator==(const DoubleWord& other) const noexcept {
        return low == other.low && high == other.high;
    }
    
    bool operator!=(const DoubleWord& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * 双宽 CAS 操作封装
 * 在支持的平台上使用硬件 128 位 CAS，否则回退到锁
 */
class DoubleWordAtomic {
private:
    alignas(16) std::atomic<DoubleWord> value_;

public:
    constexpr DoubleWordAtomic() noexcept : value_{} {}
    explicit constexpr DoubleWordAtomic(DoubleWord initial) noexcept : value_(initial) {}
    
    DoubleWordAtomic(const DoubleWordAtomic&) = delete;
    DoubleWordAtomic& operator=(const DoubleWordAtomic&) = delete;
    
    DoubleWord load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return value_.load(order);
    }
    
    void store(DoubleWord desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        value_.store(desired, order);
    }
    
    bool compare_exchange_weak(DoubleWord& expected, DoubleWord desired,
                              std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.compare_exchange_weak(expected, desired, order);
    }
    
    bool compare_exchange_strong(DoubleWord& expected, DoubleWord desired,
                                std::memory_order order = std::memory_order_seq_cst) noexcept {
        return value_.compare_exchange_strong(expected, desired, order);
    }
};

/**
 * CPU 自旋优化工具
 */
inline void cpu_relax() noexcept {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    _mm_pause();
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_ia32_pause();
#else
    std::this_thread::yield();
#endif
}

/**
 * 内存屏障工具函数
 */
inline void memory_fence() noexcept {
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

inline void acquire_fence() noexcept {
    std::atomic_thread_fence(std::memory_order_acquire);
}

inline void release_fence() noexcept {
    std::atomic_thread_fence(std::memory_order_release);
}

/**
 * 编译器屏障
 */
inline void compiler_fence() noexcept {
#if defined(_MSC_VER)
    _ReadWriteBarrier();
#elif defined(__GNUC__)
    __asm__ __volatile__("" ::: "memory");
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

} // namespace Corona::Concurrent::Core