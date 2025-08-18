#include "SceneManager.h"
#include "Components.h"
#include "Events.hpp"
#include "Global.h"

#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>

#include <format>
#include <iostream>

namespace ECS
{
    /***************** Scene *****************/
    Scene::Scene(entt::entity sceneId, void *surface, bool lightField) : dispatcher(entt::dispatcher{}),
                     animationSystem(std::make_shared<ECS::Systems::AnimationSystem>()),
                     audioSystem(std::make_shared<ECS::Systems::AudioSystem>()),
                     renderingSystem(std::make_shared<ECS::Systems::RenderingSystem>()),
                     sceneId(sceneId)
    {
        entt::entity scene = sceneId;
        Global::get().registry->emplace<ECS::Components::Camera>(scene);
        Global::get().registry->emplace<ECS::Components::SunLight>(scene);
        Global::get().registry->emplace<ECS::Components::Actors>(scene);

        animationSystem->registerEvents(dispatcher);
        audioSystem->registerEvents(dispatcher);
        renderingSystem->registerEvents(dispatcher);

        if(surface) setDisplaySurface(surface);
        dispatcher.trigger<ECS::Events::SceneCreate>();        
    }

    Scene::~Scene()
    {
        entt::entity scene = sceneId;
        dispatcher.trigger<Events::SceneDestroy>();
        Global::get().registry->destroy(scene);
        dispatcher.clear();
    }

    void Scene::setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov)
    {
        entt::entity scene = sceneId;
        auto& camera = Global::get().registry->get<Components::Camera>(scene);
    }

    void Scene::setSunDirection(const std::array<float, 3> &direction)
    {
        entt::entity scene = sceneId;
        auto& sunlight = Global::get().registry->get<Components::SunLight>(scene);
    }

    void Scene::setDisplaySurface(void *surface)
    {
        if (renderingSystem != nullptr)
        {
            std::dynamic_pointer_cast<ECS::Systems::RenderingSystem>(renderingSystem)->setDisplaySurface(surface);
            std::printf("Display surface set for scene %llu\n", static_cast<uint64_t>(sceneId));
        }
    }

    /***************** SceneManager *****************/

    SceneManager::SceneManager() : scenes({})
    {
        std::cout << "SceneManager created\n";
    }

    SceneManager::~SceneManager()
    {
        scenes.clear();
        std::cout << "SceneManager destroyed\n";
    }
    
    void SceneManager::addScene(entt::entity id, std::shared_ptr<Scene> scene)
    {
        if (scenes.contains(id))
        {
            std::cout << std::format("Scene with id {} already exists\n", static_cast<uint64_t>(id));
            return;
        }
        scenes[id] = scene;
        std::cout << std::format("Scene with id {} added\n", static_cast<uint64_t>(id));
    }

    entt::entity SceneManager::createScene(void *surface, bool lightField)
    {
        entt::entity sceneId = Global::get().registry->create();
        auto scene = std::make_shared<Scene>(sceneId, surface, lightField);
        addScene(sceneId, scene);
        return sceneId;
    }

    void SceneManager::removeScene(entt::entity id)
    {
        if (!scenes.contains(id))
        {
            std::cout << std::format("Scene with id {} does not exist\n", static_cast<uint64_t>(id));
            return;
        }
        scenes.erase(id);
        std::cout << std::format("Scene with id {} removed\n", static_cast<uint64_t>(id));
    }

    std::shared_ptr<ECS::Scene> SceneManager::getScene(entt::entity id) const
    {
        if (!scenes.contains(id))
        {
            std::cout << std::format("Scene with id {} does not exist\n", static_cast<uint64_t>(id));
            return nullptr;
        }
        return scenes.at(id);
    }

} // namespace ECS