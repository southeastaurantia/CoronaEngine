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
        auto& cache = Engine::inst().data_cache();

        while (!unhandled_data_keys.empty())
        {
            auto id = unhandled_data_keys.front();
            unhandled_data_keys.pop();

            processDisplay(id);
        }

        for (auto id: data_keys)
        {
            processDisplay(id);
        }
    }
    void DisplaySystemDefault::stop()
    {
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine