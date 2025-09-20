//
// Created by 25473 on 25-9-19.
//

#include "CoronaEngineAPI.h"

#include <memory>



namespace CoronaEngine {

    CoronaEngineAPI::Scene::Scene(void *surface, bool lightField)
        : sceneID(Corona::DataId::Next())
    {
        auto scene = std::shared_ptr<Corona::Scene>();
        auto &sceneCache = Corona::Engine::Instance().Cache<Corona::Scene>();
        sceneCache.insert(sceneID, scene);
        if (surface)
        {
            scene->displaySurface = surface;
            // render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchScene, sceneID);
            // render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::setDisplaySurface, scene);
        }
    }

    CoronaEngineAPI::Scene::~Scene()
    {
        // sceneCache.erase(sceneID);
    }

    void CoronaEngineAPI::Scene::setCamera(const ktm::fvec3 &position, const ktm::fvec3 &forward, const ktm::fvec3 &worldUp, float fov)
    {

    }

    void CoronaEngineAPI::Scene::setSunDirection(ktm::fvec3 direction)
    {
    }

    ktm::fvec3 CoronaEngineAPI::Scene::getSunDirection() const
    {
        return ktm::fvec3{};
    }

    void CoronaEngineAPI::Scene::setDisplaySurface(void *surface)
    {
        // sceneCache.modify(sceneID, [surface, this](std::shared_ptr<Corona::Scene> &scene) {
        //     scene->displaySurface = surface;
        //     render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::WatchScene, sceneID);
        //     render_queue.enqueue(&renderingSystem, &Corona::RenderingSystem::setDisplaySurface, scene);
        // });
    }

    void *CoronaEngineAPI::Scene::getDisplaySurface() const
    {
        return nullptr;
    }

    CoronaEngineAPI::Actor *CoronaEngineAPI::Scene::detectActorByRay(ktm::fvec3 origin, ktm::fvec3 dir)
    {
        return nullptr;
    }

    CoronaEngineAPI::Actor *CoronaEngineAPI::Scene::detectActorByScreen(ktm::uvec2 pixel)
    {
        return nullptr;
    }

    CoronaEngineAPI::Actor::Actor(std::string path)
    {
    }

    CoronaEngineAPI::Actor::~Actor()
    {
    }

    void CoronaEngineAPI::Actor::move(ktm::fvec3 pos)
    {
    }

    void CoronaEngineAPI::Actor::rotate(ktm::fvec3 euler)
    {
    }

    void CoronaEngineAPI::Actor::scale(ktm::fvec3 size)
    {
    }

    void CoronaEngineAPI::Actor::setPose(const Pose &newPose)
    {
    }

    CoronaEngineAPI::Actor::Pose CoronaEngineAPI::Actor::getPose() const
    {
        return pose;
    }

    void CoronaEngineAPI::Actor::setMeshShape(std::string path)
    {
    }

    void CoronaEngineAPI::Actor::setAnimation(std::string path)
    {
    }

    void CoronaEngineAPI::Actor::updateAnimation(float deltaTime)
    {

    }

    void CoronaEngineAPI::Actor::setActorMatrix(ktm::fmat4x4 matrix)
    {
    }

    ktm::fmat4x4 CoronaEngineAPI::Actor::getActorMatrix() const
    {
        return ktm::fmat4x4::from_eye();
    }

    void CoronaEngineAPI::Actor::detectCollision()
    {
    }

    void CoronaEngineAPI::Actor::setOpticsParams(const OpticsParams &params)
    {
    }

    void CoronaEngineAPI::Actor::setAcousticsParams(const AcousticsParams &params)
    {
    }

    void CoronaEngineAPI::Actor::setMechanicsParams(const MechanicsParams &params)
    {
    }
} // CoronaEngine