#pragma once

#include <ktm/type_vec.h>

namespace Corona::Components {

struct DisplaySurface {
    void* surface = nullptr;
};

struct Camera {
    float fov = 45.0f;
    ktm::fvec3 pos{};
    ktm::fvec3 forward{};
    ktm::fvec3 worldUp{};
};

struct SunDirection {
    ktm::fvec3 dir{};
};

}  // namespace Corona::Components
