//
// Created by 47226 on 2025/9/5.
//

#include "DisplaySystemDefault.hpp"

namespace CoronaEngine
{
    DisplaySystemDefault &DisplaySystemDefault::get_singleton()
    {
        static DisplaySystemDefault inst(240);
        return inst;
    }
    DisplaySystemDefault::DisplaySystemDefault(const FPS fps)
        : BaseDisplaySystem(fps)
    {
    }
    DisplaySystemDefault::~DisplaySystemDefault()
    {
    }
    const char *DisplaySystemDefault::name()
    {
        return "DisplaySystemDefault";
    }
    void DisplaySystemDefault::_start()
    {
    }
    void DisplaySystemDefault::_tick()
    {
    }
    void DisplaySystemDefault::_stop()
    {
    }
} // namespace CoronaEngine