#pragma once

#include <cstdint>

namespace Corona::Components {

struct ModelResource {
    std::uintptr_t model_handle{};
    ResourceId model_id;
    std::uintptr_t device_handle{};
};

struct Actor {
    ktm::fvec3 position{0.0f, 0.0f, 0.0f};
    ktm::fvec3 scale{1.0f, 1.0f, 1.0f};
    ktm::fquat rotation = ktm::fquat::identity();
};

struct Light {
    ktm::fvec3 position;
    ktm::fvec3 scale;
    ktm::fvec3 rotation;
};

struct Camera {
    ktm::fvec3 position;
    ktm::fvec3 forward;
    ktm::fvec3 world_up;
    float fov{};
};

}  // namespace Corona::Components
