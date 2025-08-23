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

        std::cout << std::format("Scene {} created.", entt::to_entity(scene)) << std::endl;

        event->scene_id_promise->set_value(scene);
    }

    void Core::onSceneDestroy(std::shared_ptr<Events::SceneDestroy> event)
    {
    }

    void Core::onActorCreate(std::shared_ptr<Events::ActorCreate> event)
    {
    }

    void Core::onActorDestroy(std::shared_ptr<Events::ActorDestroy> event)
    {
    }

    void Core::onSceneSetDisplaySurface(std::shared_ptr<Events::SceneSetDisplaySurface> event)
    {
        BackBridge::render_dispatcher().enqueue<std::shared_ptr<Events::SceneSetDisplaySurface>>(event);
    }
} // namespace ECS