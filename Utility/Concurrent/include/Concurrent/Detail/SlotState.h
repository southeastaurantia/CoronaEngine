#pragma once

#include <cstdint>

namespace Corona::Concurrent::Detail {

/// \brief 槽位生命周期状态，生产者与消费者通过原子方式进行状态转换。
enum class SlotState : std::uint8_t {
    kEmpty = 0,
    kWriting,
    kFull,
    kConsuming
};

} // namespace Corona::Concurrent::Detail
