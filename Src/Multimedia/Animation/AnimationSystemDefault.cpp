//
// Created by 47226 on 2025/9/4.
//

#include "AnimationSystemDefault.hpp"

#include "Core/Logger.h"

namespace Corona
{
    AnimationSystemDefault &AnimationSystemDefault::inst()
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
        LOG_DEBUG("{} started.", name());
    }
    void AnimationSystemDefault::tick()
    {
    }
    void AnimationSystemDefault::stop()
    {
        LOG_DEBUG("{} stopped.", name());
    }
} // namespace CoronaEngine