#pragma once

#include <ECS/ISystem.h>

namespace ECS::Systems
{
    class AudioSystem final : public ISystem
    {
      public:
        AudioSystem() = default;
        virtual ~AudioSystem() = default;

        const char *getName() const override;

      private:
        void onRegisterEvents(entt::dispatcher &dispatcher) override;
        void onStart() override;
        void onQuit() override;
        void mainloop() override;
    };
} // namespace ECS::Systems
