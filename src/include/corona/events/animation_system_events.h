#pragma once

namespace Corona::Events {
struct AnimationSystemDemoEvent {
    int demo_value;
};

struct EngineToAnimationDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events