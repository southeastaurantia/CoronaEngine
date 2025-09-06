//
// Created by 47226 on 2025/9/5.
//

#include "DisplaySystemDefault.hpp"

#include "Utils/CabbageLogger.hpp"

namespace CoronaEngine
{
    DisplaySystemDefault &DisplaySystemDefault::get_singleton()
    {
        static DisplaySystemDefault inst;
        return inst;
    }

    const char *DisplaySystemDefault::name()
    {
        return "DisplaySystemDefault";
    }
    DisplaySystemDefault::DisplaySystemDefault()
    {
    }
    DisplaySystemDefault::~DisplaySystemDefault()
    {
    }
    void DisplaySystemDefault::start()
    {
        LOG_DEBUG(std::format("{} started", name()));
    }
    void DisplaySystemDefault::tick()
    {
    }
    void DisplaySystemDefault::stop()
    {
        LOG_DEBUG(std::format("{} stopped", name()));
    }
} // namespace CoronaEngine