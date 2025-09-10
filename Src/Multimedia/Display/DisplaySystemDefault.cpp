//
// Created by 47226 on 2025/9/5.
//

#include "DisplaySystemDefault.hpp"

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

                // 计算当前帧执行所花费的时间（微秒）
                auto frameTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();

                // 计算需要等待的时间以达到目标帧率
                auto waitTimeUs = TARGET_FRAME_TIME_US - frameTimeUs;

                // 如果计算结果为正，则等待相应时间
                if (waitTimeUs > 0)
                {
                    // 使用高精度sleep等待，确保微秒级精度
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