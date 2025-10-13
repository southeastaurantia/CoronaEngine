#include "DisplaySystem.h"
#include "Engine.h"

using namespace Corona;

DisplaySystem::DisplaySystem()
    : ThreadedSystem("DisplaySystem")
{
    Engine::instance().add_queue(name(), std::make_unique<SafeCommandQueue>());
}

void DisplaySystem::onStart()
{
}
void DisplaySystem::onTick()
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
void DisplaySystem::onStop()
{
}
void DisplaySystem::process_display(uint64_t /*id*/)
{
}
