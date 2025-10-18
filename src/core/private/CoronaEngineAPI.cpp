//
// Created by 25473 on 25-9-19.
//

#include <CoronaEngineAPI.h>

#include <memory>

#include "Engine.h"
#include "components/SceneComponents.h"
#include "SceneEvents.h"
#include "EventBus.h"
#include "components/ActorComponents.h"
#include "events/ActorEvents.h"

// 定义静态 ECS 注册表
entt::registry CoronaEngineAPI::registry_;

CoronaEngineAPI::Scene::Scene(void* surface, bool /*lightField*/)
    : sceneID(registry_.create()) {
    registry_.emplace<RenderTag>(sceneID);
    if (surface) {
        registry_.emplace_or_replace<Corona::Components::DisplaySurface>(sceneID, Corona::Components::DisplaySurface{surface});
        // 事件广播：通知渲染系统显示表面更新（副本传递）
        const std::uint64_t sid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(sceneID));
        Corona::Engine::instance().events<Corona::SceneEvents::DisplaySurfaceUpdated>()
            .publish("scene.surface", Corona::SceneEvents::DisplaySurfaceUpdated{ sid, surface });
    }
}

CoronaEngineAPI::Scene::~Scene() {
    // 事件广播：通知渲染系统该场景移除，渲染系统可清理本地快照
    const std::uint64_t sid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(sceneID));
    Corona::Engine::instance().events<Corona::SceneEvents::Removed>()
        .publish("scene.removed", Corona::SceneEvents::Removed{ sid });

    registry_.destroy(sceneID);
}

void CoronaEngineAPI::Scene::setCamera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& worldUp, float fov) const {
    registry_.emplace_or_replace<Corona::Components::Camera>(sceneID, Corona::Components::Camera{ fov, position, forward, worldUp });

    // 事件广播：摄像机参数副本
    const std::uint64_t sid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(sceneID));
    Corona::Engine::instance().events<Corona::SceneEvents::CameraUpdated>()
        .publish("scene.camera", Corona::SceneEvents::CameraUpdated{ sid, fov, position, forward, worldUp });
}

void CoronaEngineAPI::Scene::setSunDirection(ktm::fvec3 direction) const {
    registry_.emplace_or_replace<Corona::Components::SunDirection>(sceneID, Corona::Components::SunDirection{ direction });

    // 事件广播：太阳光方向副本
    const std::uint64_t sid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(sceneID));
    Corona::Engine::instance().events<Corona::SceneEvents::SunUpdated>()
        .publish("scene.sun", Corona::SceneEvents::SunUpdated{ sid, direction });
}

void CoronaEngineAPI::Scene::setDisplaySurface(void* surface) {
    registry_.emplace_or_replace<Corona::Components::DisplaySurface>(sceneID, Corona::Components::DisplaySurface{ surface });

    // 事件广播：显示表面更新副本
    const std::uint64_t sid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(sceneID));
    Corona::Engine::instance().events<Corona::SceneEvents::DisplaySurfaceUpdated>()
        .publish("scene.surface", Corona::SceneEvents::DisplaySurfaceUpdated{ sid, surface });
}

CoronaEngineAPI::Actor::Actor(const std::string& path)
    : actorID(registry_.create()) {
    // 标签（可选）
    registry_.emplace<RenderTag>(actorID);
    // 仅存资源ID/路径为组件
    registry_.emplace_or_replace<Corona::Components::ModelResource>(actorID, Corona::Components::ModelResource{path});

    // 广播：Actor 生成（携带资源路径），由系统（如 RenderingSystem）订阅后自行加载
    const std::uint64_t aid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(actorID));
    Corona::Engine::instance().events<Corona::ActorEvents::Spawned>()
        .publish("actor.spawn", Corona::ActorEvents::Spawned{ aid, path });
}

CoronaEngineAPI::Actor::~Actor() {
    const std::uint64_t aid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(actorID));

    // 广播：Actor 移除
    Corona::Engine::instance().events<Corona::ActorEvents::Removed>()
        .publish("actor.removed", Corona::ActorEvents::Removed{ aid });

    // 可选：移除标签/组件
    // registry_.remove_if_exists<RenderTag>(actorID);
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 pos) const {
    const std::uint64_t aid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(actorID));
    Corona::Engine::instance().events<Corona::ActorEvents::TransformUpdated>()
        .publish("actor.transform", Corona::ActorEvents::TransformUpdated{ aid, pos, {}, {} });
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 euler) const {
    const std::uint64_t aid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(actorID));
    Corona::Engine::instance().events<Corona::ActorEvents::TransformUpdated>()
        .publish("actor.transform", Corona::ActorEvents::TransformUpdated{ aid, {}, euler, {} });
}

void CoronaEngineAPI::Actor::scale(ktm::fvec3 size) const {
    const std::uint64_t aid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(actorID));
    Corona::Engine::instance().events<Corona::ActorEvents::TransformUpdated>()
        .publish("actor.transform", Corona::ActorEvents::TransformUpdated{ aid, {}, {}, size });
}
