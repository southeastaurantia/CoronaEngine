#pragma once

#include <cstdint>

namespace Corona::Events {

/**
 * @brief 引擎内部事件（单线程使用 EventBus）
 */
struct EngineDemoEvent {
    int demo_value;
};

/**
 * @brief 帧开始事件（广播到所有系统，使用 EventStream）
 */
struct FrameBeginEvent {
    uint64_t frame_number;
    float delta_time;
};

/**
 * @brief 帧结束事件（广播到所有系统，使用 EventStream）
 */
struct FrameEndEvent {
    uint64_t frame_number;
    float frame_time;
};

/**
 * @brief 引擎关闭请求事件（广播到所有系统，使用 EventStream）
 */
struct EngineShutdownEvent {
    // Empty event
};

}  // namespace Corona::Events