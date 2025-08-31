#pragma once

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
        std::shared_ptr<entt::registry> registry;

        AnimationSystem animation_system;
        AudioSystem audio_system;
        RenderingSystem rendering_system;
        ResourceManager resource_manager;

        void onSceneCreate(Events::SceneCreateRequest event);
        void onSceneDestroy(Events::SceneDestroy event);
        void onSceneSetDisplaySurface(Events::SceneSetDisplaySurface event);
        void onSceneAddActor(Events::SceneAddActor event);
        void onSceneRemoveActor(Events::SceneRemoveActor event);
        void onSceneSetCamera(Events::SceneSetCamera event);

        void onActorCreate(Events::ActorCreateRequest event);
        void onActorDestroy(Events::ActorDestroy event);
        void onActorRotate(Events::ActorRotate event);
    };
} // namespace ECS