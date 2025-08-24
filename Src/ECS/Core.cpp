#include "Core.h"

#include "BackBridge.h"
#include "Components.h"
#include "FrontBridge.h"

#include <format>

namespace ECS
{
    Core::Core()
        : registry(std::make_shared<entt::registry>()),
          animation_system(registry),
          audio_system(registry),
          rendering_system(registry),
          resource_manager(registry)
    {
        // TODO: FrontBridge事件注册
        FrontBridge::dispatcher().sink<Events::SceneCreateRequest>().connect<&Core::onSceneCreate>(this);
        FrontBridge::dispatcher().sink<Events::SceneDestroy>().connect<&Core::onSceneDestroy>(this);
        FrontBridge::dispatcher().sink<Events::SceneSetDisplaySurface>().connect<&Core::onSceneSetDisplaySurface>(this);
        FrontBridge::dispatcher().sink<Events::SceneAddActor>().connect<&Core::onSceneAddActor>(this);
        FrontBridge::dispatcher().sink<Events::SceneRemoveActor>().connect<&Core::onSceneRemoveActor>(this);
        FrontBridge::dispatcher().sink<Events::SceneSetCamera>().connect<&Core::onSceneSetCamera>(this);

        FrontBridge::dispatcher().sink<Events::ActorCreateRequest>().connect<&Core::onActorCreate>(this);
        FrontBridge::dispatcher().sink<Events::ActorDestroy>().connect<&Core::onActorDestroy>(this);
        FrontBridge::dispatcher().sink<Events::ActorRotate>().connect<&Core::onActorRotate>(this);

        std::cout << "Core started" << std::endl;
    };

    Core::~Core()
    {
        std::cout << "Core stoped" << std::endl;
    }

    void Core::onSceneCreate(Events::SceneCreateRequest event)
    {
        auto scene = registry->create();
        registry->emplace<Components::Camera>(scene, Components::Camera{});
        registry->emplace<Components::SunLight>(scene, Components::SunLight{});
        registry->emplace<Components::Actors>(scene, Components::Actors{});

        std::cout << std::format("Scene {} created.", entt::to_entity(scene)) << std::endl;

        event.scene_id_promise->set_value(scene);
    }

    void Core::onSceneDestroy(Events::SceneDestroy event)
    {
        for (const auto &actors = registry->get<Components::Actors>(event.scene).data;
             const auto &actor : actors)
        {
            auto &scenes = registry->get<Components::SceneRef>(actor).scenes;
            scenes.erase(event.scene);
            std::cout << std::format("Scene {} removed reference from actor {} before scene destroyed.", entt::to_entity(event.scene), entt::to_entity(actor)) << std::endl;
        }

        registry->destroy(event.scene);
        std::cout << std::format("Scene {} destroyed.", entt::to_entity(event.scene)) << std::endl;
    }

    void Core::onSceneSetDisplaySurface(Events::SceneSetDisplaySurface event)
    {
        BackBridge::render_dispatcher().enqueue(event);
    }

    void Core::onSceneAddActor(Events::SceneAddActor event)
    {
        auto &actors = registry->get<Components::Actors>(event.scene).data;
        if (std::ranges::find(actors, event.actor) != actors.end())
        {
            return;
        }
        actors.push_back(event.actor);

        auto &[scenes] = registry->get<Components::SceneRef>(event.actor);
        scenes.insert(event.scene);

        std::cout << std::format("Actor {} added to Scene {}.", entt::to_entity(event.actor), entt::to_entity(event.scene)) << std::endl;
    }

    void Core::onSceneRemoveActor(Events::SceneRemoveActor event)
    {
        auto &actors = registry->get<Components::Actors>(event.scene).data;

        if (std::ranges::find(actors, event.actor) == actors.end())
        {
            return;
        }

        std::erase(actors, event.actor);

        auto &scenes = registry->get<Components::SceneRef>(event.actor).scenes;
        scenes.erase(event.scene);

        std::cout << std::format("Actor {} removed from Scene {}", entt::to_entity(event.actor), entt::to_entity(event.scene)) << std::endl;
    }

    void Core::onSceneSetCamera(Events::SceneSetCamera event)
    {
        auto &camera = registry->get<Components::Camera>(event.scene);
        camera.pos = {event.pos[0], event.pos[1], event.pos[2]};
        camera.forward = {event.forward[0], event.forward[1], event.forward[2]};
        camera.worldUp = {event.worldup[0], event.worldup[1], event.worldup[2]};
        camera.fov = event.fov;
    }

    void Core::onActorCreate(Events::ActorCreateRequest event)
    {
        auto actor = registry->create();
        registry->emplace<Components::ActorPose>(actor, Components::ActorPose{});
        registry->emplace<Components::Model>(actor, Components::Model{});
        registry->emplace<Components::SceneRef>(actor, Components::SceneRef{});

        std::cout << std::format("Actor {} created.", entt::to_entity(actor)) << std::endl;
        event.actor_id_promise->set_value(actor);
    }

    void Core::onActorDestroy(Events::ActorDestroy event)
    {
        for (const auto &scenes = registry->get<Components::SceneRef>(event.actor).scenes;
             const auto &scene : scenes)
        {
            auto &actors = registry->get<Components::Actors>(scene).data;
            std::erase(actors, event.actor);
            std::cout << std::format("Actor {} removed from scene {} before actor destroyed.", entt::to_entity(event.actor), entt::to_entity(scene)) << std::endl;
        }

        registry->destroy(event.actor);
        std::cout << std::format("Actor {} destroyed.", entt::to_entity(event.actor)) << std::endl;
    }

    void Core::onActorRotate(Events::ActorRotate event)
    {
        auto &pose = registry->get<Components::ActorPose>(event.actor);
        pose.rotate = {
            event.euler[0],
            event.euler[1],
            event.euler[2]};
    }

} // namespace ECS