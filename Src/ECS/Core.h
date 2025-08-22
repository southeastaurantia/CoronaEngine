#pragma once

#include "TaskScheduler.h"

#include <ECS/Events.hpp>
#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>
#include <Resource/ResourceManager.h>

#include <entt/entt.hpp>

namespace ECS
{
    class Core final
    {
      public:
        static constexpr int FPS = 120;
        static constexpr float MinFrameTime = 1.0f / FPS;

        Core();
        ~Core();

      private:
        bool running;
        std::thread coreThread;
        std::shared_ptr<entt::registry> registry;

        AnimationSystem animation_system;
        AudioSystem audio_system;
        RenderingSystem rendering_system;
        ResourceManager resource_manager;

        void coreLoop();

        void onSceneCreate(Events::SceneCreate &event);
        void onSceneDestroy(const Events::SceneDestroy &event);
        void onActorCreate(const Events::ActorCreate &event);
        void onActorDestroy(const Events::ActorDestroy &event);
        void onSceneSetDisplaySurface(const Events::SceneSetDisplaySurface &event);
    };
} // namespace ECS