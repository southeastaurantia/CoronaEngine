#pragma once

#include <CabbageDisplayer.h>
#include <ECS/Events.hpp>
#include <ECS/ISystem.h>


namespace ECS::Systems
{
    class RenderingSystem final : public ISystem
    {
      public:
        RenderingSystem() = default;
        virtual ~RenderingSystem() = default;

        const char *getName() const override;
        void setDisplaySurface(const ECS::Events::SetDisplaySurface &event);

      private:
        void onStart() override;
        void onQuit() override;
        void mainloop() override;

        void displayLoop();

      private:
        std::unique_ptr<std::thread> displayThread;
        HardwareDisplayer displayManager;
    };
} // namespace ECS::Systems
