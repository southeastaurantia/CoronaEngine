//
// Created by 47226 on 2025/9/5.
//

#include "RenderingSystemDefault.hpp"

namespace CoronaEngine
{
    RenderingSystemDefault &RenderingSystemDefault::get_singleton()
    {
        static RenderingSystemDefault inst(120);
        return inst;
    }
    RenderingSystemDefault::RenderingSystemDefault(const FPS fps)
        : BaseRenderingSystem(fps)
    {
    }
    RenderingSystemDefault::~RenderingSystemDefault()
    {
    }
    const char *RenderingSystemDefault::name()
    {
        return "RenderingSystemDefault";
    }
    void RenderingSystemDefault::_start()
    {
    }
    void RenderingSystemDefault::_tick()
    {
        
    }
    void RenderingSystemDefault::_stop()
    {
    }
} // namespace CoronaEngine