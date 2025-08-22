#pragma once

#include "TaskScheduler.h"

#include <ECS/Events.hpp>

#include <entt/entt.hpp>

namespace ECS
{
    class Core final
    {
      public:
        static constexpr int FPS = 120;
        static constexpr float MinFrameTime = 1.0f / FPS;

        static std::shared_ptr<entt::registry> registry();

        Core();
        ~Core();

      private:
        bool running;
        std::thread coreThread;

        void coreLoop();

        void onSceneCreate(Events::SceneCreate &event);
        void onSceneDestroy(const Events::SceneDestroy &event);
        void onActorCreate(const Events::ActorCreate &event);
        void onActorDestroy(const Events::ActorDestroy &event);
        void onSceneSetDisplaySurface(const Events::SceneSetDisplaySurface &event);
    };
} // namespace ECS