//
// Created by 47226 on 2025/9/5.
//

#include "AudioSystemDefault.hpp"

#include "Utils/CabbageLogger.hpp"

namespace CoronaEngine
{
    AudioSystemDefault &AudioSystemDefault::get_singleton()
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
        LOG_DEBUG(std::format("{} started", name()));
    }
    void AudioSystemDefault::tick()
    {
    }
    void AudioSystemDefault::stop()
    {
        LOG_DEBUG(std::format("{} stopped", name()));
    }
} // namespace CoronaEngine