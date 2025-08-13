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
    Scene::Scene() : dispatcher(entt::dispatcher{}),
                     animationSystem(std::make_shared<ECS::Systems::AnimationSystem>()),
                     audioSystem(std::make_shared<ECS::Systems::AudioSystem>()),
                     renderingSystem(std::make_shared<ECS::Systems::RenderingSystem>())
    {
        animationSystem->registerEvents(dispatcher);
        audioSystem->registerEvents(dispatcher);
        renderingSystem->registerEvents(dispatcher);

        dispatcher.trigger<ECS::Events::SceneCreate>();
    }

    Scene::~Scene()
    {
        dispatcher.trigger<ECS::Events::SceneDestroy>();
        dispatcher.clear();
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