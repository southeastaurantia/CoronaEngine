#pragma once

#include "SceneManager.h"
#include "TaskScheduler.h"

#include <Resource/ResourceManager.h>

#include <entt/entt.hpp>

namespace ECS
{
    class Singleton final
    {
      public:
        static Singleton &get();

        std::shared_ptr<entt::dispatcher> dispatcher;
        std::shared_ptr<entt::registry> registry;
        std::shared_ptr<ECS::SceneManager> sceneMgr;
        std::shared_ptr<ECS::ResourceManager> resourceMgr;
        std::shared_ptr<ECS::TaskScheduler> scheduler;

      private:
        Singleton();
        ~Singleton();

        void mainloop();

        bool running{true};
        std::unique_ptr<std::thread> mainloopThread;
    };
} // namespace ECS