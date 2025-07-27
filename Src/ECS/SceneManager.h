#pragma once

#include "Scene.h"
#include <unordered_map>

namespace ECS
{
    class SceneManager final
    {
      private:
        SceneManager() = delete;
        SceneManager(const SceneManager &) = delete;
        SceneManager(const SceneManager &&) = delete;
        SceneManager &operator=(const SceneManager &) = delete;
        SceneManager &operator=(const SceneManager &&) = delete;

      public:
        static entt::entity createScene();
        static void removeScene(const entt::entity &scene);

      private:
        static std::unordered_map<entt::entity, std::unique_ptr<ECS::Scene>> scenes;
    };

} // namespace ECS