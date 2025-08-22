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
        void *surface;
        bool lightField;
    };

    struct SceneDestroy
    {
        entt::entity scene;
    };

    struct SceneAddActor
    {
        entt::entity scene;
        entt::entity actor;
    };

    struct SceneRemoveActor
    {
        entt::entity scene;
        entt::entity actor;
    };

    struct ActorCreate
    {
        entt::entity scene;
        std::string path;
    };

    struct ActorDestroy
    {
        entt::entity scene;
        entt::entity actor;
    };

    struct SceneSetDisplaySurface
    {
        entt::entity scene;
        void *surface;
    };
} // namespace ECS::Events