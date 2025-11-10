#pragma once

namespace Corona::Events {

/**
 * @brief 动画系统内部事件（单线程使用 EventBus）
 */
struct AnimationSystemDemoEvent {
    int demo_value;
};

/**
 * @brief 引擎到动画系统的跨线程事件（使用 EventStream）
 */
struct EngineToAnimationDemoEvent {
    float delta_time;
};

/**
 * @brief 动画系统到引擎的跨线程事件（使用 EventStream）
 */
struct AnimationToEngineDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events