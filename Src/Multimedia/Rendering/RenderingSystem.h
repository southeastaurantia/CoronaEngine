#pragma once

#include <ECS/ISystem.h>
#include <CabbageDisplayer.h>

namespace ECS::Systems
{
    class RenderingSystem final : public ISystem
    {
      public:
        RenderingSystem() = default;
        virtual ~RenderingSystem() = default;

        const char *getName() const override;
        void setDisplaySurface(void *surface);

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
