//
// Created by 47226 on 2025/9/5.
//

#include "AudioSystemDefault.hpp"

namespace CabbageFW
{
    AudioSystemDefault &AudioSystemDefault::get_singleton()
    {
        static AudioSystemDefault inst(120);
        return inst;
    }
    AudioSystemDefault::AudioSystemDefault(const FPS fps)
        : BaseAudioSystem(fps)
    {
    }
    AudioSystemDefault::~AudioSystemDefault()
    {
    }

    const char *AudioSystemDefault::name()
    {
        return "AudioSystemDefault";
    }
    void AudioSystemDefault::_start()
    {
    }
    void AudioSystemDefault::_tick()
    {
    }
    void AudioSystemDefault::_stop()
    {
    }
} // namespace CabbageFW