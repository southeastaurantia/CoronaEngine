#pragma once

namespace Corona::Events {
struct DisplaySystemDemoEvent {
    int demo_value;
};
struct EngineToDisplayDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events