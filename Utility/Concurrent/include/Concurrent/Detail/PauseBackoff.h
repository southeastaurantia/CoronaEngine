#pragma once

#include <chrono>
#include <thread>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <immintrin.h>
#endif

namespace Corona::Concurrent::Detail {

/// \brief 指数退避自旋器，短期活跃轮询后逐步退化为 yield / sleep。
class PauseBackoff {
public:
    PauseBackoff() noexcept = default;

    /// \brief 重置内部自旋计数，在新的等待轮次开始前调用。
    void reset() noexcept { spin_count_ = 0; }

    /// \brief 根据当前自旋次数选择 pause / yield 或短暂 sleep。
    void pause() noexcept {
        if (spin_count_ < kActiveSpinLimit) {
            ++spin_count_;
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
            _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
            __builtin_ia32_pause();
#else
            // 对于非 x86 架构退化为空循环。
            std::this_thread::yield();
            return;
#endif
        } else if (spin_count_ < kYieldLimit) {
            ++spin_count_;
            std::this_thread::yield();
        } else {
            ++spin_count_;
            std::this_thread::sleep_for(std::chrono::nanoseconds(50));
        }
    }

private:
    static constexpr int kActiveSpinLimit = 64;
    static constexpr int kYieldLimit = 256;
    int spin_count_ = 0;
};

} // namespace Corona::Concurrent::Detail
