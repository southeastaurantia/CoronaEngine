#include "Core.h"

#include "Components.h"

#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>

#include <format>

static AnimationSystem animation_system(ECS::Core::registry());
static AudioSystem audio_system(ECS::Core::registry());
static RenderingSystem rendering_system(ECS::Core::registry());

namespace ECS
{
    std::shared_ptr<entt::registry> Core::registry()
    {
        static std::shared_ptr<entt::registry> inst;
        return inst;
    }

    Core::Core()
        : running(true)
    {
        // TODO: FrontBridge事件注册

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