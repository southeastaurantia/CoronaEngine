#pragma once

namespace Corona::Events {

struct EngineDemoEvent {
    int demo_value;
};

struct AnimationToEngineDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events