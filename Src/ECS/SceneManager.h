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
        std::shared_ptr<ECS::ISystem> animationSystem;
        std::shared_ptr<ECS::ISystem> audioSystem;
        std::shared_ptr<ECS::ISystem> renderingSystem;
    };

    class SceneManager final
    {
      public:
        SceneManager();
        ~SceneManager();

      private:
        std::unordered_map<entt::entity, std::shared_ptr<ECS::Scene>> scenes;
    };

} // namespace ECS