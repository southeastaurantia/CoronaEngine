//
// Created by 47226 on 2025/9/5.
//

#include "AudioSystemDefault.hpp"

#include "Core/Logger.h"

namespace Corona
{
    AudioSystemDefault &AudioSystemDefault::inst()
    {
        static AudioSystemDefault inst;
        return inst;
    }
    const char *AudioSystemDefault::name()
    {
        return "AudioSystemDefault";
    }
    AudioSystemDefault::AudioSystemDefault()
    {
    }
    AudioSystemDefault::~AudioSystemDefault()
    {
    }

    void AudioSystemDefault::start()
    {
        LOG_DEBUG("{} started", name());
    }
    void AudioSystemDefault::tick()
    {
        auto& cache = Engine::inst().data_cache();

        while (!unhandled_data_keys.empty())
        {
            auto id = unhandled_data_keys.front();
            unhandled_data_keys.pop();

            processAudio(id);
        }

        for (auto id: data_keys)
        {
            processAudio(id);
        }
    }
    void AudioSystemDefault::stop()
    {
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine