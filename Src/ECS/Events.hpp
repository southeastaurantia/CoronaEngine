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

    struct CreateSceneEntity
    {
        entt::entity scene;
        void *surface;
        bool lightField;
    };

    struct DestroySceneEntity
    {
        entt::entity scene;
    };

    struct CreateActorEntity
    {
        entt::entity scene;
        entt::entity actor;
        std::string path;
    };

    struct DestroyActorEntity
    {
        entt::entity scene;
        entt::entity actor;
    };
} // namespace ECS::Events