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

        void onSceneCreate(std::shared_ptr<Events::SceneCreateRequest> event);
        void onSceneDestroy(std::shared_ptr<Events::SceneDestroy> event);
        void onSceneSetDisplaySurface(std::shared_ptr<Events::SceneSetDisplaySurface> event);
        void onSceneAddActor(std::shared_ptr<Events::SceneAddActor> event);
        void onSceneRemoveActor(std::shared_ptr<Events::SceneRemoveActor> event);

        void onActorCreate(std::shared_ptr<Events::ActorCreateRequest> event);
        void onActorDestroy(std::shared_ptr<Events::ActorDestroy> event);
    };
} // namespace ECS