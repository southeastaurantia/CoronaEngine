#pragma once

namespace Corona::Events {

/**
 * @brief 力学系统内部事件（单线程使用 EventBus）
 */
struct MechanicsSystemDemoEvent {
    int demo_value;
};

/**
 * @brief 引擎到力学系统的跨线程事件（使用 EventStream）
 */
struct EngineToMechanicsDemoEvent {
    float delta_time;
};

/**
 * @brief 力学系统到引擎的跨线程事件（使用 EventStream）
 */
struct MechanicsToEngineDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events
