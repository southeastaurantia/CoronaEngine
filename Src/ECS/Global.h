#pragma once

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
        static std::unique_ptr<entt::dispatcher> dispatcher;
        static std::unique_ptr<entt::registry> registry;
    };
} // namespace ECS