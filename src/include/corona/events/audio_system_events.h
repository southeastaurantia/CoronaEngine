#pragma once

namespace Corona::Events {
struct AudioSystemDemoEvent {
    int demo_value;
};

struct EngineToAudioDemoEvent {
    float delta_time;
};

}  // namespace Corona::Events