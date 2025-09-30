//
// Created by 25473 on 25-9-19.
//

#include "CoronaEngineAPI.h"

#include <memory>


CoronaEngineAPI::Scene::Scene(void *surface, bool lightField)
    : sceneID(Corona::DataId::Next())
{
    auto scene = std::make_shared<Corona::Scene>();
    auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
    sceneCache.insert(sceneID, scene);
    if (surface)
    {
        sceneCache.modify(sceneID, [surface](const std::shared_ptr<Corona::Scene> &scene) {
            scene->displaySurface = surface;
        });
        auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
        auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchScene, sceneID);
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::setDisplaySurface, scene);
    }
}

CoronaEngineAPI::Scene::~Scene()
{
    auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
    auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
    auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::UnwatchScene, sceneID);
    sceneCache.erase(sceneID);
}

void CoronaEngineAPI::Scene::setCamera(const ktm::fvec3 &position, const ktm::fvec3 &forward, const ktm::fvec3 &worldUp, float fov) const
{
    auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
    sceneCache.modify(sceneID, [position, forward, worldUp, fov](const std::shared_ptr<Corona::Scene> &scene) {
        scene->camera.pos = position;
        scene->camera.forward = forward;
        scene->camera.worldUp = worldUp;
        scene->camera.fov = fov;
    });

}

void CoronaEngineAPI::Scene::setSunDirection(ktm::fvec3 direction) const
{
    auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
    sceneCache.modify(sceneID, [direction](const std::shared_ptr<Corona::Scene> &scene) {
        scene->sunDirection = direction;
    });
}

void CoronaEngineAPI::Scene::setDisplaySurface(void *surface)
{
    auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
    auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
    auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
    sceneCache.modify(sceneID, [surface, &renderingSystem, &render_queue](const std::shared_ptr<Corona::Scene> &scene) {
        scene->displaySurface = surface;
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::setDisplaySurface, scene);
    });
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchScene, sceneID);
}


CoronaEngineAPI::Actor::Actor(const std::string &path)
    : actorID(Corona::DataId::Next()),
    animationID(0)
{
    const auto res = Corona::Engine::Instance().Resources().load({"model", path});
    auto model = std::dynamic_pointer_cast<Corona::Model>(res);
    if (!model)
    {
        CE_LOG_WARN("Failed to load model: %s", path.c_str());
        return;
    }
    auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
    modelCache.insert(actorID, model);

    if (!model->skeletalAnimations.empty())
    {
        auto animState = std::make_shared<Corona::AnimationState>();
        animState->model = model;
        animState->animationIndex = 0;

        animationID = Corona::DataId::Next();

        auto &animStateCache = Corona::Engine::Instance().Cache<Corona::AnimationState>();
        auto &animationSystem = Corona::Engine::Instance().GetSystem<Corona::AnimationSystem>();
        auto &anim_queue = Corona::Engine::Instance().GetQueue(animationSystem.name());

        animStateCache.insert(animationID, animState);
        anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::WatchState, animationID);

        auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
        auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchModel, actorID);
        return;
    }

    auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
    auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
    render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchModel, actorID);
}

CoronaEngineAPI::Actor::~Actor()
{
    if (animationID != 0)
    {
        auto &animStateCache = Corona::Engine::Instance().Cache<Corona::AnimationState>();
        auto &animationSystem = Corona::Engine::Instance().GetSystem<Corona::AnimationSystem>();
        auto &anim_queue = Corona::Engine::Instance().GetQueue(animationSystem.name());
        anim_queue.enqueue(&animationSystem, &Corona::AnimationSystem::UnwatchState, animationID);
        animStateCache.erase(animationID);
    }
    auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
    if (modelCache.get(actorID) != nullptr)
    {
        auto &renderingSystem = Corona::Engine::Instance().GetSystem<Corona::RenderingSystem>();
        auto &render_queue = Corona::Engine::Instance().GetQueue(renderingSystem.name());
        render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::UnwatchModel, actorID);
        modelCache.erase(actorID);
    }
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 pos) const
{
    auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
    modelCache.modify(actorID, [pos](const std::shared_ptr<Corona::Model> &model) {
        model->positon = pos;
    });
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 euler) const
{
    auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
    modelCache.modify(actorID, [euler](const std::shared_ptr<Corona::Model> &model) {
        model->rotation = euler;
    });
}

void CoronaEngineAPI::Actor::scale(ktm::fvec3 size) const
{
    auto &modelCache = Corona::Engine::Instance().Cache<Corona::Model>();
    modelCache.modify(actorID, [size](const std::shared_ptr<Corona::Model> &model) {
        model->scale = size;
    });
}

