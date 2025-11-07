#pragma once

#include <ktm/type_vec.h>

namespace Corona::Components {

struct DisplaySurface {
    void* surface = nullptr;
};

struct Storage {
    std::vector<entt::entity> actors;
    std::vector<entt::entity> cameras;
    std::vector<entt::entity> lights;
};

struct SunDirection {
    ktm::fvec3 dir{};
};

}  // namespace Corona::Components
