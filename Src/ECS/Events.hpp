#pragma once

#include <entt/entt.hpp>

namespace ECS::Events
{
    struct EngineStart
    {
    };

    struct EngineStop
    {
    };

    struct SceneCreate
    {
    };

    struct SceneDestroy
    {
    };

    struct CreateActorEntity
    {
        entt::entity scene;
    };
} // namespace ECS::Events