#include "Core.h"

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
        FrontBridge::dispatcher().sink<ECS::Events::SceneCreate>().connect<&Core::onSceneCreate>(this);

        coreThread = std::thread(&Core::coreLoop, this);

        std::puts("Core started.");
    };

    Core::~Core()
    {
        running = false;

        if (coreThread.joinable())
        {
            coreThread.join();
        }

        std::puts("Core stoped.");
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

    void Core::onSceneCreate(Events::SceneCreate &event)
    {
        auto scene = registry->create();

        std::puts(std::format("Scene {} created.", entt::to_entity(scene)).c_str());

        // TODO: 使用promise和future将scene id返回给前端
    }

    void Core::onSceneDestroy(const Events::SceneDestroy &event)
    {
    }

    void Core::onActorCreate(const Events::ActorCreate &event)
    {
    }

    void Core::onActorDestroy(const Events::ActorDestroy &event)
    {
    }

    void Core::onSceneSetDisplaySurface(const Events::SceneSetDisplaySurface &event)
    {
    }
} // namespace ECS