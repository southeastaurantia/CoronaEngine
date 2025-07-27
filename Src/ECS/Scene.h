#pragma once

#include "ISystem.h"

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
}; // namespace ECS