#include "AudioSystem.h"

#include <chrono>

namespace ECS::Systems
{
    const char *AudioSystem::getName() const
    {
        return "AudioSystem";
    }

    void AudioSystem::onStart()
    {
    }

    void AudioSystem::onQuit()
    {
    }

    void AudioSystem::mainloop()
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
} // namespace ECS::Systems
