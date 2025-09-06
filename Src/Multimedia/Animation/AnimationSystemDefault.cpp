//
// Created by 47226 on 2025/9/4.
//

#include "AnimationSystemDefault.hpp"

#include "Utils/CabbageLogger.hpp"

namespace CoronaEngine
{
    AnimationSystemDefault &AnimationSystemDefault::get_singleton()
    {
        static AnimationSystemDefault inst;
        return inst;
    }
    const char *AnimationSystemDefault::name()
    {
        return "AnimationSystemDefault";
    }
    AnimationSystemDefault::AnimationSystemDefault()
    {
    }
    AnimationSystemDefault::~AnimationSystemDefault()
    {
    }
    void AnimationSystemDefault::start()
    {
        LOG_DEBUG(std::format("{} started.", name()));
    }
    void AnimationSystemDefault::tick()
    {
    }
    void AnimationSystemDefault::stop()
    {
        LOG_DEBUG(std::format("{} stopped.", name()));
    }
} // namespace CoronaEngine