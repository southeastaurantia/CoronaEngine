#pragma once

#include <cstdint>
#include <string>
#include <ktm/type_vec.h>

namespace Corona::ActorEvents {
struct Spawned {
    std::uint64_t actorId;
    std::string   modelPath; // 资源路径或标识
};

struct Removed {
    std::uint64_t actorId;
};

struct TransformUpdated {
    std::uint64_t actorId;
    ktm::fvec3    pos{};
    ktm::fvec3    rot{};
    ktm::fvec3    scale{};
};
}