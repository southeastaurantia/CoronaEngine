#pragma once

namespace Corona::Events {

/**
 * @brief 动画系统内部事件（单线程使用 EventBus）
 */
struct KinematicsSystemDemoEvent {
    int demo_value;
};

/**
 * @brief 引擎到动画系统的跨线程事件（使用 EventStream）
 */
struct EngineToKinematicsDemoEvent {
    float delta_time;
};

/**
 * @brief 动画系统到引擎的跨线程事件（使用 EventStream）
 */
struct KinematicsToEngineDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events