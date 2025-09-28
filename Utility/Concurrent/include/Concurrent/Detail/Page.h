#pragma once

#include "Common.h"
#include "Slot.h"

#include <array>
#include <atomic>
#include <cstddef>

namespace Corona::Concurrent::Detail {

/// \brief 固定大小的槽位页，承载一组顺序票据对应的 Slot。
template <typename T>
struct Page {
    using SlotType = Slot<T>;
    static constexpr std::size_t kSlotCount = ItemsPerPage<T>();

    Page() noexcept { reset(0); }

    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;

    /// \brief 重置页面状态并绑定新的起始票据索引。
    void reset(std::size_t new_index) noexcept {
        base_index = new_index;
        consumed.store(0, std::memory_order_relaxed);
        for (auto& slot : slots) {
            slot.mark_empty();
        }
        next_free = nullptr;
    }

    /// \brief 判断所有槽位均已消费完成。
    bool fully_consumed() const noexcept {
        return consumed.load(std::memory_order_acquire) == kSlotCount;
    }

    SlotType* slot_at(std::size_t index) noexcept { return &slots[index]; }
    const SlotType* slot_at(std::size_t index) const noexcept { return &slots[index]; }

    std::size_t base_index = 0;
    std::atomic<std::size_t> consumed{0};
    Page* next_free = nullptr;
    std::array<SlotType, kSlotCount> slots;
};

} // namespace Corona::Concurrent::Detail
