#pragma once

#include "SceneManager.h"

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
    };
} // namespace ECS