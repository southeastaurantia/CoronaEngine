#include "AudioSystem.h"
#include "Core/Engine/Engine.h"

using namespace Corona;

AudioSystem::AudioSystem()
    : ThreadedSystem("AudioSystem")
{
    Engine::Instance().AddQueue(name(), std::make_unique<SafeCommandQueue>());
}

void AudioSystem::onStart()
{
}
void AudioSystem::onTick()
{
    auto &rq = Engine::Instance().GetQueue(name());
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
void AudioSystem::processAudio(uint64_t /*id*/)
{
}
