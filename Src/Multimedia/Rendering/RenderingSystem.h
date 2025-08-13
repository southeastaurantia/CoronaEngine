#pragma once

#include <ECS/ISystem.h>

namespace ECS::Systems
{
    class RenderingSystem final : public ISystem
    {
      public:
        RenderingSystem() = default;
        virtual ~RenderingSystem() = default;

        const char *getName() const override;

      private:
        void onRegisterEvents(entt::dispatcher &dispatcher) override;
        void onStart() override;
        void onQuit() override;
        void mainloop() override;

        void displayLoop();

      private:
        std::unique_ptr<std::thread> displayThread;
    };
} // namespace ECS::Systems
