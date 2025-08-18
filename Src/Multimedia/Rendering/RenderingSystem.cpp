#include "RenderingSystem.h"

#include <chrono>

namespace ECS::Systems
{
    const char *RenderingSystem::getName() const
    {
        return "RenderingSystem";
    }

    void RenderingSystem::setDisplaySurface(void *surface)
    {
        HardwareDisplayer displayManager(surface);
        HardwareImage finalOutputImage(ktm::uvec2(800, 800), ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
        displayManager = finalOutputImage;
    }

    void RenderingSystem::onRegisterEvents(entt::dispatcher &dispatcher)
    {
    }

    void RenderingSystem::onStart()
    {
        this->displayThread = std::make_unique<std::thread>(&RenderingSystem::displayLoop, this);
    }

    void RenderingSystem::onQuit()
    {
        if (displayThread != nullptr)
        {
            displayThread->join();
        }
    }

    void RenderingSystem::mainloop()
    {
        static constexpr float MaxFrameTime = 1.0f / 120.0f;

        while (isRunning())
        {
            auto startTime = std::chrono::high_resolution_clock::now();

            /********** Do Something **********/

            /********** Do Something **********/

            auto endTime = std::chrono::high_resolution_clock::now();
            auto frameTime = std::chrono::duration<float>(endTime - startTime).count();

            if (frameTime < MaxFrameTime)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MaxFrameTime - frameTime) * 1000.0f)));
            }
        }
    }

    void RenderingSystem::displayLoop()
    {
        static constexpr float MaxFrameTime = 1.0f / 240.0f;

        while (isRunning())
        {
            auto startTime = std::chrono::high_resolution_clock::now();

            /********** Do Something **********/

            /********** Do Something **********/

            auto endTime = std::chrono::high_resolution_clock::now();
            auto frameTime = std::chrono::duration<float>(endTime - startTime).count();

            if (frameTime < MaxFrameTime)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MaxFrameTime - frameTime) * 1000.0f)));
            }
        }
    }
} // namespace ECS::Systems
