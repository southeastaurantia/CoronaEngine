#include "AudioSystem.h"
#include "Engine.h"

using namespace Corona;

AudioSystem::AudioSystem()
    : ThreadedSystem("AudioSystem")
{
    Engine::instance().add_queue(name(), std::make_unique<SafeCommandQueue>());
}

void AudioSystem::onStart()
{
}
void AudioSystem::onTick()
{
    auto &rq = Engine::instance().get_queue(name());
    int spun = 0;
    while (spun < 100 && !rq.empty())
    {
        if (!rq.try_execute())
            continue;
        ++spun;
    }
}
void AudioSystem::onStop()
{
}
void AudioSystem::process_audio(uint64_t /*id*/)
{
}
