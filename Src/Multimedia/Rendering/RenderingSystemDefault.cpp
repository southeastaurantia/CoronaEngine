//
// Created by 47226 on 2025/9/5.
//

#include "RenderingSystemDefault.hpp"

#include "Utils/CabbageLogger.hpp"

namespace CoronaEngine
{
    RenderingSystemDefault &RenderingSystemDefault::get_singleton()
    {
        static RenderingSystemDefault inst;
        return inst;
    }
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
        LOG_DEBUG(std::format("{} started", name()));
    }
    void RenderingSystemDefault::tick()
    {
    }
    void RenderingSystemDefault::stop()
    {
        LOG_DEBUG(std::format("{} stopped", name()));
    }
} // namespace CoronaEngine