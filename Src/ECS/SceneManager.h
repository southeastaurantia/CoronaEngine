#pragma once

#include "ISystem.h"

#include <unordered_map>

namespace ECS
{
    class Scene final
    {
      public:
        Scene();
        ~Scene();

      private:
        entt::dispatcher dispatcher;
        std::unique_ptr<ECS::ISystem> animationSystem;
        std::unique_ptr<ECS::ISystem> audioSystem;
        std::unique_ptr<ECS::ISystem> renderingSystem;
    };

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