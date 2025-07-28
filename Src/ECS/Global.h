#pragma once

#include "SceneManager.h"
#include "TaskScheduler.h"

#include <Resource/ResourceManager.h>

#include <entt/entt.hpp>

namespace ECS
{
    class Global final
    {
      private:
        Global() = delete;
        Global(const Global &) = delete;
        Global(const Global &&) = delete;
        Global &operator=(const Global &) = delete;
        Global &operator=(const Global &&) = delete;

      public:
        static std::shared_ptr<entt::dispatcher> Dispatcher;
        static std::shared_ptr<entt::registry> Registry;
        static std::shared_ptr<ECS::SceneManager> SceneMgr;
        static std::shared_ptr<ECS::ResourceManager> ResourceMgr;
        static std::shared_ptr<ECS::TaskScheduler> TaskScheduler;
    };
} // namespace ECS