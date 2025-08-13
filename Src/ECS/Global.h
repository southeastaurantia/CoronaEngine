#pragma once

#include "SceneManager.h"
#include "TaskScheduler.h"

#include <ECS/Events.hpp>
#include <Resource/ResourceManager.h>

#include <entt/entt.hpp>

namespace ECS
{
    class Global final
    {
      public:
        static Global &get();

        std::shared_ptr<entt::dispatcher> dispatcher;
        std::shared_ptr<entt::registry> registry;
        std::shared_ptr<ECS::SceneManager> sceneMgr;
        std::shared_ptr<ECS::ResourceManager> resourceMgr;
        std::shared_ptr<ECS::TaskScheduler> scheduler;

      private:
        Global();
        ~Global();

        void mainloop();

        bool running{true};
        std::unique_ptr<std::thread> mainloopThread;

        void onCreateSceneEntity(const ECS::Events::CreateSceneEntity &event);
        void onDestroySceneEntity(const ECS::Events::DestroySceneEntity &event);
        void onCreatActorEntity(const ECS::Events::CreateActorEntity &event);
        void onDestroyActorEntity(const ECS::Events::DestroyActorEntity &event);
    };
} // namespace ECS