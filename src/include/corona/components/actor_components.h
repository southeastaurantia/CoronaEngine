#pragma once

#include <cstdint>

namespace Corona::Components {

struct ModelResource {
    std::uintptr_t model_handle{};
    ResourceId model_id;
    std::uintptr_t device_handle{};
};

struct Actor {
    ktm::fvec3 position;
    ktm::fvec3 scale;
    ktm::fvec3 rotation;
};

struct Light {
    ktm::fvec3 position;
    ktm::fvec3 scale;
    ktm::fvec3 rotation;
};

struct Camera {
    ktm::fvec3 position;
    ktm::fvec3 forward;
    ktm::fvec3 worldUp;
    float fov{};
};

}  // namespace Corona::Components
