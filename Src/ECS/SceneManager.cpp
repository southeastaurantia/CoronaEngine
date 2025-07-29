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
    }

    SceneManager::~SceneManager()
    {
        scenes.clear();
    }

} // namespace ECS