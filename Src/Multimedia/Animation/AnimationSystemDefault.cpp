//
// Created by 47226 on 2025/9/4.
//

#include "AnimationSystemDefault.hpp"

namespace CabbageFW
{
    AnimationSystemDefault &AnimationSystemDefault::get_singleton()
    {
        static AnimationSystemDefault inst(120);
        return inst;
    }
    AnimationSystemDefault::AnimationSystemDefault(const FPS fps)
        : BaseAnimationSystem(fps)
    {
    }
    AnimationSystemDefault::~AnimationSystemDefault()
    {
    }
    const char *AnimationSystemDefault::name()
    {
        return "AnimationSystemDefault";
    }
    void AnimationSystemDefault::_start()
    {
    }
    void AnimationSystemDefault::_tick()
    {
    }
    void AnimationSystemDefault::_stop()
    {
    }
} // namespace CabbageFW