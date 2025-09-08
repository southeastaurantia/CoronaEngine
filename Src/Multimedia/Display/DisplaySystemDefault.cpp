//
// Created by 47226 on 2025/9/5.
//

#include "DisplaySystemDefault.hpp"

#include "Core/Logger.h"

namespace Corona
{
    DisplaySystemDefault &DisplaySystemDefault::inst()
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
        LOG_DEBUG("{} started", name());
    }
    void DisplaySystemDefault::tick()
    {
    }
    void DisplaySystemDefault::stop()
    {
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine