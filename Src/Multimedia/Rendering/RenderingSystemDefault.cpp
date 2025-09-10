//
// Created by 47226 on 2025/9/5.
//

#include "RenderingSystemDefault.h"

#include "Core/Logger.h"

namespace Corona
{
    const char *RenderingSystemDefault::name()
    {
        return "RenderingSystemDefault";
    }
    RenderingSystemDefault::RenderingSystemDefault()
        : running(false)
    {
    }
    RenderingSystemDefault::~RenderingSystemDefault()
    {
    }
    void RenderingSystemDefault::start()
    {
        constexpr int64_t TARGET_FRAME_TIME_US = 1000000 / 120;
        running.store(true);
        renderThread = std::thread([&] {
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
    void RenderingSystemDefault::tick()
    {
    }
    void RenderingSystemDefault::stop()
    {
        running.store(false);
        if (renderThread.joinable())
        {
            renderThread.join();
        }
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine