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
        this->createSceneEntity(); // Create default scene
    }

    SceneManager::~SceneManager()
    {
        for (auto &[scene, _] : scenes)
        {
            this->destroySceneEntity(scene);
        }

        scenes.clear();
    }

    entt::entity SceneManager::createSceneEntity()
    {
        entt::entity scene = ECS::Global::Registry->create();
        ECS::Global::Registry->emplace<ECS::Components::Camera>(scene, ECS::Components::Camera{});
        ECS::Global::Registry->emplace<ECS::Components::SunLight>(scene, ECS::Components::SunLight{});
        ECS::Global::Registry->emplace<ECS::Components::Actors>(scene, ECS::Components::Actors{});
        scenes[scene] = std::make_shared<ECS::Scene>();
        std::cout << std::format("Scene {} created\n", static_cast<uint64_t>(scene));
        return scene;
    }

    void SceneManager::destroySceneEntity(const entt::entity &scene)
    {
        if (scene == entt::null)
        {
            std::cout << "Cannot destroy null scene\n";
            return;
        }
        if (!scenes.contains(scene))
        {
            std::cout << std::format("Cannot destroy scene {}, scene does not exist\n", static_cast<uint64_t>(scene));
            return;
        }

        std::cout << std::format("Destroying scene {}\n", static_cast<uint64_t>(scene));
        scenes.erase(scene);
        std::cout << std::format("Scene {} removed from the map\n", static_cast<uint64_t>(scene));

        auto &actorsComponent = ECS::Global::Registry->get<ECS::Components::Actors>(scene);
        for (auto &actor : actorsComponent.actors)
        {
            ECS::Global::Registry->destroy(actor);
        }
        ECS::Global::Registry->destroy(scene);
    }

} // namespace ECS