//
// Created by 47226 on 2025/9/5.
//

#include "AudioSystemDefault.h"

#include "Core/Logger.h"

namespace Corona
{
    const char *AudioSystemDefault::name()
    {
        return "AudioSystemDefault";
    }
    AudioSystemDefault::AudioSystemDefault()
        : running(false)
    {
    }
    AudioSystemDefault::~AudioSystemDefault()
    {
    }

    void AudioSystemDefault::start()
    {
        running.store(true);
        constexpr int64_t TARGET_FRAME_TIME_US = 1000000 / 120;

        audioThread = std::thread([&] {
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
    void AudioSystemDefault::tick()
    {

    }
    void AudioSystemDefault::stop()
    {
        running.store(false);
        if (audioThread.joinable())
        {
            audioThread.join();
        }
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine