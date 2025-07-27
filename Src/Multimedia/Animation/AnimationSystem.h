#pragma once

#include <ECS/ISystem.h>

namespace ECS::Systems
{
    class AnimationSystem final : public ISystem
    {
      public:
        AnimationSystem() = default;
        ~AnimationSystem() = default;

        const char *getName() const override;

      private:
        void onRegisterEvents(entt::dispatcher &dispatcher) override;
        void onStart() override;
        void onQuit() override;
        void mainloop() override;
    };
} // namespace ECS::Systems