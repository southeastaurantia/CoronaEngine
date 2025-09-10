//
// Created by 47226 on 2025/9/5.
//

#include "DisplaySystemDefault.h"

#include "Core/Logger.h"

namespace Corona
{
    const char *DisplaySystemDefault::name()
    {
        return "DisplaySystemDefault";
    }
    DisplaySystemDefault::DisplaySystemDefault()
        : running(false)
    {
    }
    DisplaySystemDefault::~DisplaySystemDefault()
    {
    }
    void DisplaySystemDefault::start()
    {
        constexpr int64_t TARGET_FRAME_TIME_US = 1000000 / 120;
        running.store(true);
        displayThread = std::thread([&] {
            while (running.load())
            {
                auto begin = std::chrono::high_resolution_clock::now();
                tick();
                auto end = std::chrono::high_resolution_clock::now();
                auto frameTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
                auto waitTimeUs = TARGET_FRAME_TIME_US - frameTimeUs;

                if (waitTimeUs > 0)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(waitTimeUs));
                }
            }
        });
        LOG_DEBUG("{} started", name());
    }
    void DisplaySystemDefault::tick()
    {
    }
    void DisplaySystemDefault::stop()
    {
        running.store(false);
        if (displayThread.joinable())
        {
            displayThread.join();
        }
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine