#pragma once

namespace Corona::Events {

/**
 * @brief 几何系统内部事件（单线程使用 EventBus）
 */
struct GeometrySystemDemoEvent {
    int demo_value;
};

/**
 * @brief 引擎到几何系统的跨线程事件（使用 EventStream）
 */
struct EngineToGeometryDemoEvent {
    float delta_time;
};

/**
 * @brief 几何系统到引擎的跨线程事件（使用 EventStream）
 */
struct GeometryToEngineDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events
