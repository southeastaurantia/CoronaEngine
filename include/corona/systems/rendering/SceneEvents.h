#pragma once

#include <ktm/type_vec.h>

#include <cstdint>

namespace Corona::SceneEvents {

struct CameraUpdated {
    std::uint64_t sceneId;
    float fov;
    ktm::fvec3 pos;
    ktm::fvec3 forward;
    ktm::fvec3 worldUp;
};

struct SunUpdated {
    std::uint64_t sceneId;
    ktm::fvec3 dir;
};

struct DisplaySurfaceUpdated {
    std::uint64_t sceneId;
    void* surface;
};

struct Removed {
    std::uint64_t sceneId;
};

}  // namespace Corona::SceneEvents
