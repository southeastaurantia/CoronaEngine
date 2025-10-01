#include "DisplaySystem.h"
#include "Core/Engine/Engine.h"

using namespace Corona;

DisplaySystem::DisplaySystem()
    : ThreadedSystem("DisplaySystem")
{
    Engine::Instance().AddQueue(name(), std::make_unique<SafeCommandQueue>());
}

void DisplaySystem::onStart()
{
}
void DisplaySystem::onTick()
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
void DisplaySystem::onStop()
{
}
void DisplaySystem::processDisplay(uint64_t /*id*/)
{
}
