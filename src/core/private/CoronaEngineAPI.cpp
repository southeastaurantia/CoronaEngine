//
// Created by 25473 on 25-9-19.
//

#include <CoronaEngineAPI.h>

#include <memory>

#include "AnimationState.h"
#include "AnimationSystem.h"
#include "Engine.h"
#include "Model.h"
#include "RenderingSystem.h"
#include "Scene.h"

CoronaEngineAPI::Scene::Scene(void* surface, bool lightField)
    : sceneID(Corona::DataId::next()) {
    auto scene = std::make_shared<Corona::Scene>();
    auto& scene_cache = Corona::Engine::instance().cache<Corona::Scene>();
    scene_cache.insert(sceneID, scene);
    if (surface) {
        scene_cache.modify(sceneID, [surface](const std::shared_ptr<Corona::Scene>& scene) {
            scene->displaySurface = surface;
        });
        auto& rendering_system = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
        auto& render_queue = Corona::Engine::instance().get_queue(rendering_system.name());
        render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::watch_scene, sceneID);
        render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::set_display_surface, scene);
    }
}

CoronaEngineAPI::Scene::~Scene() {
    auto& scene_cache = Corona::Engine::instance().cache<Corona::Scene>();
    auto& rendering_system = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
    auto& render_queue = Corona::Engine::instance().get_queue(rendering_system.name());
    render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::unwatch_scene, sceneID);
    scene_cache.erase(sceneID);
}

void CoronaEngineAPI::Scene::setCamera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& worldUp, float fov) const {
    auto& scene_cache = Corona::Engine::instance().cache<Corona::Scene>();
    scene_cache.modify(sceneID, [position, forward, worldUp, fov](const std::shared_ptr<Corona::Scene>& scene) {
        scene->camera.pos = position;
        scene->camera.forward = forward;
        scene->camera.worldUp = worldUp;
        scene->camera.fov = fov;
    });
}

void CoronaEngineAPI::Scene::setSunDirection(ktm::fvec3 direction) const {
    auto& scene_cache = Corona::Engine::instance().cache<Corona::Scene>();
    scene_cache.modify(sceneID, [direction](const std::shared_ptr<Corona::Scene>& scene) {
        scene->sunDirection = direction;
    });
}

void CoronaEngineAPI::Scene::setDisplaySurface(void* surface) {
    auto& scene_cache = Corona::Engine::instance().cache<Corona::Scene>();
    auto& rendering_system = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
    auto& render_queue = Corona::Engine::instance().get_queue(rendering_system.name());
    scene_cache.modify(sceneID, [surface, &rendering_system, &render_queue](const std::shared_ptr<Corona::Scene>& scene) {
        scene->displaySurface = surface;
        render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::set_display_surface, scene);
    });
    render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::watch_scene, sceneID);
}

CoronaEngineAPI::Actor::Actor(const std::string& path)
    : actorID(Corona::DataId::next()),
      animationID(0) {
    const auto res = Corona::Engine::instance().resources().load({"model", path});
    auto model = std::dynamic_pointer_cast<Corona::Model>(res);
    if (!model) {
        CE_LOG_WARN("Failed to load model: %s", path.c_str());
        return;
    }
    auto& model_cache = Corona::Engine::instance().cache<Corona::Model>();
    model_cache.insert(actorID, model);

    if (!model->skeletalAnimations.empty()) {
        auto animState = std::make_shared<Corona::AnimationState>();
        animState->model = model;
        animState->animationIndex = 0;

        animationID = Corona::DataId::next();

        auto& anim_state_cache = Corona::Engine::instance().cache<Corona::AnimationState>();
        auto& animation_system = Corona::Engine::instance().get_system<Corona::AnimationSystem>();
        auto& anim_queue = Corona::Engine::instance().get_queue(animation_system.name());

        anim_state_cache.insert(animationID, animState);
        anim_queue.enqueue(&animation_system, &Corona::AnimationSystem::watch_state, animationID);

        auto& rendering_system = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
        auto& render_queue = Corona::Engine::instance().get_queue(rendering_system.name());
        render_queue.enqueue(&rendering_system, &Corona::RenderingSystem::watch_model, actorID);
        return;
    }

    auto& animationSystem = Corona::Engine::instance().get_system<Corona::AnimationSystem>();
    auto& anim_queue = Corona::Engine::instance().get_queue(animationSystem.name());
    anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::watch_model, actorID);

    auto& renderingSystem = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
    auto& render_queue = Corona::Engine::instance().get_queue(renderingSystem.name());
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::watch_model, actorID);
}

CoronaEngineAPI::Actor::~Actor() {
    if (animationID != 0) {
        auto& anim_state_cache = Corona::Engine::instance().cache<Corona::AnimationState>();
        auto& animation_system = Corona::Engine::instance().get_system<Corona::AnimationSystem>();
        auto& anim_queue = Corona::Engine::instance().get_queue(animation_system.name());
        anim_queue.enqueue(&animation_system, &Corona::AnimationSystem::unwatch_state, animationID);
        anim_state_cache.erase(animationID);
    }
    auto& model_cache = Corona::Engine::instance().cache<Corona::Model>();
    if (model_cache.get(actorID) != nullptr) {
        auto& animationSystem = Corona::Engine::instance().get_system<Corona::AnimationSystem>();
        auto& anim_queue = Corona::Engine::instance().get_queue(animationSystem.name());
        anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::unwatch_model, actorID);

        auto& renderingSystem = Corona::Engine::instance().get_system<Corona::RenderingSystem>();
        auto& render_queue = Corona::Engine::instance().get_queue(renderingSystem.name());
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::unwatch_model, actorID);
        model_cache.erase(actorID);
    }
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 pos) const {
    auto& model_cache = Corona::Engine::instance().cache<Corona::Model>();
    model_cache.modify(actorID, [pos](const std::shared_ptr<Corona::Model>& model) {
        model->positon = pos;
    });
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 euler) const {
    auto& model_cache = Corona::Engine::instance().cache<Corona::Model>();
    model_cache.modify(actorID, [euler](const std::shared_ptr<Corona::Model>& model) {
        model->rotation = euler;
    });
}

void CoronaEngineAPI::Actor::scale(ktm::fvec3 size) const {
    auto& model_cache = Corona::Engine::instance().cache<Corona::Model>();
    model_cache.modify(actorID, [size](const std::shared_ptr<Corona::Model>& model) {
        model->scale = size;
    });
}
