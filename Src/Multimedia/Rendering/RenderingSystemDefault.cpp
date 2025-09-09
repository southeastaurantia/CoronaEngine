//
// Created by 47226 on 2025/9/5.
//

#include "RenderingSystemDefault.hpp"

#include "Core/Logger.h"

namespace Corona
{
    const char *RenderingSystemDefault::name()
    {
        return "RenderingSystemDefault";
    }
    RenderingSystemDefault::RenderingSystemDefault()
    {
    }
    RenderingSystemDefault::~RenderingSystemDefault()
    {
    }
    void RenderingSystemDefault::start()
    {
        LOG_DEBUG("{} started", name());
    }
    void RenderingSystemDefault::tick()
    {
    }
    void RenderingSystemDefault::stop()
    {
        LOG_DEBUG("{} stopped", name());
    }
} // namespace CoronaEngine