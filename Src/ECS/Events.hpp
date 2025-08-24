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

    struct SceneSetDisplaySurface
    {
        entt::entity scene;
        void *surface;
    };

    struct SceneSetCamera
    {
        entt::entity scene;
        std::array<float, 3> pos;
        std::array<float, 3> forward;
        std::array<float, 3> worldup;
        float fov;
    };

    struct ActorCreateRequest
    {
        std::string path;
        std::shared_ptr<std::promise<entt::entity>> actor_id_promise; // 后端创建ID返回给前端
    };

    struct ActorDestroy
    {
        entt::entity actor;
    };

    struct ActorRotate
    {
        entt::entity actor;
        std::array<float, 3> euler;
    };

} // namespace ECS::Events