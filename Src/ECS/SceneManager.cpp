#include "SceneManager.h"
#include "Events.hpp"
#include "Global.h"

#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>

#include <iostream>

std::unordered_map<entt::entity, std::unique_ptr<ECS::Scene>> ECS::SceneManager::scenes = {};

namespace ECS
{
    /***************** Scene *****************/
    Scene::Scene() : dispatcher(entt::dispatcher{}),
                     animationSystem(std::make_unique<ECS::Systems::AnimationSystem>()),
                     audioSystem(std::make_unique<ECS::Systems::AudioSystem>()),
                     renderingSystem(std::make_unique<ECS::Systems::RenderingSystem>())
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

    /***************** Scene Manager *****************/
    entt::entity SceneManager::createScene()
    {
        return entt::null;
    }

    void SceneManager::removeScene(const entt::entity &scene)
    {
        if (scene == entt::null)
        {
            std::cout << "Cannot remove null scene\n";
            return;
        }
    }

} // namespace ECS