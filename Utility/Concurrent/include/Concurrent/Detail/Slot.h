#pragma once

#include "PauseBackoff.h"
#include "SlotState.h"

#include <atomic>
#include <memory>
#include <new>
#include <utility>

namespace Corona::Concurrent::Detail {

/// \brief 单个元素槽位，负责协调构造/销毁及状态同步。
template <typename T>
class Slot {
public:
    using value_type = T;

    Slot() noexcept : state_(SlotState::kEmpty) {}

    Slot(const Slot&) = delete;
    Slot& operator=(const Slot&) = delete;

    /// \brief 尝试从 Empty 状态转为 Writing，失败表示其他线程正在操作。
    bool try_acquire_empty() noexcept {
        auto expected = SlotState::kEmpty;
        return state_.compare_exchange_strong(expected, SlotState::kWriting, std::memory_order_acquire, std::memory_order_relaxed);
    }

    /// \brief 自旋等待直至槽位可写。
    void wait_until_empty(PauseBackoff& backoff) noexcept {
        while (!try_acquire_empty()) {
            if (state_.load(std::memory_order_acquire) == SlotState::kEmpty) {
                continue;
            }
            backoff.pause();
        }
    }

    template <typename Alloc, typename U>
    /// \brief 使用 allocator 在槽位中构造元素副本，并标记为 Full。
    void construct(Alloc& alloc, U&& value) {
        using traits = std::allocator_traits<Alloc>;
        traits::construct(alloc, storage_ptr(), std::forward<U>(value));
        state_.store(SlotState::kFull, std::memory_order_release);
    }

    template <typename Alloc, typename... Args>
    /// \brief 在槽位内部原地构造元素。
    void emplace(Alloc& alloc, Args&&... args) {
        using traits = std::allocator_traits<Alloc>;
        traits::construct(alloc, storage_ptr(), std::forward<Args>(args)...);
        state_.store(SlotState::kFull, std::memory_order_release);
    }

    template <typename Alloc>
    /// \brief 销毁槽位内对象，并将状态重置为 Empty。
    void destroy(Alloc& alloc) noexcept {
        using traits = std::allocator_traits<Alloc>;
        traits::destroy(alloc, storage_ptr());
        state_.store(SlotState::kEmpty, std::memory_order_release);
    }

    /// \brief 读取当前状态，供调试或回收使用。
    SlotState state() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    /// \brief 等待直到生产者写入完毕。
    void wait_until_full(PauseBackoff& backoff) const noexcept {
        while (state_.load(std::memory_order_acquire) != SlotState::kFull) {
            backoff.pause();
        }
    }

    T& value() noexcept { return *storage_ptr(); }
    const T& value() const noexcept { return *storage_ptr(); }

    /// \brief 将状态标记为 Consuming，提示其他线程进入消费阶段。
    void mark_consuming() noexcept { state_.store(SlotState::kConsuming, std::memory_order_release); }

    /// \brief 直接切换为 Empty，一般用于复位流程。
    void mark_empty() noexcept { state_.store(SlotState::kEmpty, std::memory_order_release); }

private:
    T* storage_ptr() noexcept { return std::launder(reinterpret_cast<T*>(&storage_)); }
    const T* storage_ptr() const noexcept { return std::launder(reinterpret_cast<const T*>(&storage_)); }

    alignas(T) unsigned char storage_[sizeof(T)];
    std::atomic<SlotState> state_;
};

} // namespace Corona::Concurrent::Detail
