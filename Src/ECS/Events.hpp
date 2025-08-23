#pragma once

#include <entt/entt.hpp>

#include <future>

namespace ECS::Events
{
    struct SceneCreateRequest
    {
        void *surface;
        bool lightField;
        std::shared_ptr<std::promise<entt::entity>> scene_id_promise; // 后端创建ID返回给前端
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

    struct ActorCreateRequest
    {
        entt::entity scene;
        std::string path;
        std::shared_ptr<std::promise<entt::entity>> actor_id_promise; // 后端创建ID返回给前端
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