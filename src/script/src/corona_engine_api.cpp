//
// Created by 25473 on 25-9-19.
//

#include <corona/api/corona_engine_api.h>
#include <corona/components/actor_components.h>
#include <corona/components/scene_components.h>

#include <memory>

// 定义静态 ECS 注册表
entt::registry CoronaEngineAPI::registry_;

CoronaEngineAPI::Scene::Scene(void* surface, bool /*lightField*/)
    : scene_id_(registry_.create()) {
    registry_.emplace<RenderTag>(scene_id_);
    if (surface) {
        registry_.emplace_or_replace<Corona::Components::DisplaySurface>(scene_id_, Corona::Components::DisplaySurface{surface});
    }
}

CoronaEngineAPI::Scene::~Scene() {
    registry_.destroy(scene_id_);
}

void CoronaEngineAPI::Scene::set_camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov) const {
    registry_.emplace_or_replace<Corona::Components::Camera>(scene_id_, Corona::Components::Camera{fov, position, forward, world_up});
}

void CoronaEngineAPI::Scene::set_sun_direction(ktm::fvec3 direction) const {
    registry_.emplace_or_replace<Corona::Components::SunDirection>(scene_id_, Corona::Components::SunDirection{direction});
}

void CoronaEngineAPI::Scene::set_display_surface(void* surface) {
    registry_.emplace_or_replace<Corona::Components::DisplaySurface>(scene_id_, Corona::Components::DisplaySurface{surface});
}

CoronaEngineAPI::Actor::Actor(const std::string& path)
    : actor_id_(registry_.create()) {
    // 标签（可选）
    registry_.emplace<RenderTag>(actor_id_);
    // 仅存资源ID/路径为组件
    registry_.emplace_or_replace<Corona::Components::ModelResource>(actor_id_, Corona::Components::ModelResource{path});
}

CoronaEngineAPI::Actor::~Actor() {
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 /*pos*/) const {
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 /*euler*/) const {
}

void CoronaEngineAPI::Actor::scale(ktm::fvec3 /*size*/) const {
}
