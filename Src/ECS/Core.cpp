#include "Core.h"

#include "BackBridge.h"
#include "Components.h"
#include "FrontBridge.h"

#include <format>

namespace ECS
{
    Core::Core()
        : running(true),
          registry(std::make_shared<entt::registry>()),
          animation_system(registry),
          audio_system(registry),
          rendering_system(registry),
          resource_manager(registry)
    {
        // TODO: FrontBridge事件注册
        FrontBridge::dispatcher().sink<std::shared_ptr<Events::SceneCreateRequest>>().connect<&Core::onSceneCreate>(this);
        FrontBridge::dispatcher().sink<std::shared_ptr<Events::SceneDestroy>>().connect<&Core::onSceneDestroy>(this);
        FrontBridge::dispatcher().sink<std::shared_ptr<Events::SceneSetDisplaySurface>>().connect<&Core::onSceneSetDisplaySurface>(this);
        FrontBridge::dispatcher().sink<std::shared_ptr<Events::SceneAddActor>>().connect<&Core::onSceneAddActor>(this);
        FrontBridge::dispatcher().sink<std::shared_ptr<Events::SceneRemoveActor>>().connect<&Core::onSceneRemoveActor>(this);

        FrontBridge::dispatcher().sink<std::shared_ptr<Events::ActorCreateRequest>>().connect<&Core::onActorCreate>(this);
        FrontBridge::dispatcher().sink<std::shared_ptr<Events::ActorDestroy>>().connect<&Core::onActorDestroy>(this);

        coreThread = std::thread(&Core::coreLoop, this);

        std::cout << "Core started" << std::endl;
    };

    Core::~Core()
    {
        running = false;

        if (coreThread.joinable())
        {
            coreThread.join();
        }

        FrontBridge::dispatcher().update();

        std::cout << "Core stoped" << std::endl;
    }

    void Core::coreLoop()
    {
        while (true)
        {
            if (!running)
            {
                break;
            }

            auto startTime = std::chrono::high_resolution_clock::now();
            FrontBridge::dispatcher().update();
            /********** Do Something **********/

            /********** Do Something **********/

            auto endTime = std::chrono::high_resolution_clock::now();

            if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < MinFrameTime)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MinFrameTime - frameTime) * 1000.0f)));
            }
        }
    }

    void Core::onSceneCreate(std::shared_ptr<Events::SceneCreateRequest> event)
    {
        auto scene = registry->create();
        registry->emplace<Components::Camera>(scene, Components::Camera{});
        registry->emplace<Components::SunLight>(scene, Components::SunLight{});
        registry->emplace<Components::Actors>(scene, Components::Actors{});

        std::cout << std::format("Scene {} created.", entt::to_entity(scene)) << std::endl;

        event->scene_id_promise->set_value(scene);
    }

    void Core::onSceneDestroy(std::shared_ptr<Events::SceneDestroy> event)
    {
        if(registry->try_get<Components::Actors>(event->scene))
        {
            auto &actors = registry->get<Components::Actors>(event->scene);

            auto actorsCopy = actors.data;
            for (auto actor : actorsCopy)
            {
                auto removeEvent = std::make_shared<Events::SceneRemoveActor>();
                removeEvent->scene = event->scene;
                removeEvent->actor = actor;
                onSceneRemoveActor(removeEvent);
            }
        }

        registry->destroy(event->scene);

        std::cout << std::format("Scene {} destroyed.", entt::to_entity(event->scene)) << std::endl;
    }

    void Core::onSceneSetDisplaySurface(std::shared_ptr<Events::SceneSetDisplaySurface> event)
    {
        BackBridge::render_dispatcher().enqueue(event);
    }

    void Core::onSceneAddActor(std::shared_ptr<Events::SceneAddActor> event)
    {
        auto &actors = registry->get<Components::Actors>(event->scene);
        actors.data.push_back(event->actor);

        if (!registry->try_get<Components::SceneRef>(event->actor))
        {
            registry->emplace<Components::SceneRef>(event->actor);
        }
        auto &sceneRef = registry->get<Components::SceneRef>(event->actor);
        sceneRef.scenes.insert(event->scene);

        std::cout << std::format("Actor {} added to Scene {}.", entt::to_entity(event->actor), entt::to_entity(event->scene)) << std::endl;
    }

    void Core::onSceneRemoveActor(std::shared_ptr<Events::SceneRemoveActor> event)
    {
        if (registry->valid(event->scene) && registry->valid(event->actor))
        {
            if (registry->try_get<Components::Actors>(event->scene))
            {
                auto &actors = registry->get<Components::Actors>(event->scene);
                actors.data.erase(std::remove(actors.data.begin(), actors.data.end(), event->actor), actors.data.end());
            }

            if (registry->try_get<Components::SceneRef>(event->actor))
            {
                auto &sceneRef = registry->get<Components::SceneRef>(event->actor);
                sceneRef.scenes.erase(event->scene);
            }

            std::cout << std::format("Actor {} removed from Scene {}", entt::to_entity(event->actor), entt::to_entity(event->scene)) << std::endl;
        }
    }

    void Core::onActorCreate(std::shared_ptr<Events::ActorCreateRequest> event)
    {
        auto actor = registry->create();

        std::cout << std::format("Actor {} created.", entt::to_entity(actor)) << std::endl;

        event->actor_id_promise->set_value(actor);
    }

    void Core::onActorDestroy(std::shared_ptr<Events::ActorDestroy> event)
    {
        registry->destroy(event->actor);

        std::cout << std::format("Actor {} destroyed.", entt::to_entity(event->actor)) << std::endl;
    }

} // namespace ECS