#pragma once

#include <entt/entt.hpp>

#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>

namespace ECS
{
    class Scene final
    {
      public:
        Scene();
        ~Scene();

      private:
        entt::dispatcher dispatcher;
        std::unique_ptr<ECS::Systems::AnimationSystem> animationSystem;
        std::unique_ptr<ECS::Systems::AudioSystem> audioSystem;
        std::unique_ptr<ECS::Systems::RenderingSystem> renderingSystem;
    };
}; // namespace ECS