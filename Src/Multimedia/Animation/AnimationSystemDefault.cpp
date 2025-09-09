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
        auto& cache = Engine::inst().data_cache();

        while (!unhandled_data_keys.empty())
        {
            auto id = unhandled_data_keys.front();
            unhandled_data_keys.pop();

            processAnimation(id);
        }

        for (auto id: data_keys)
        {
            processAnimation(id);
        }
    }
    void AnimationSystemDefault::stop()
    {
        LOG_DEBUG("{} stopped.", name());
    }

    void AnimationSystemDefault::processAnimation(DataCache::id_type id)
    {
        // auto& cache = Engine::inst().data_cache();
        // auto it = cache.actor_pose.find(id);
        //
        // if (it != cache.actor_pose.end())
        // {
        //     auto&
        // }
    }
} // namespace CoronaEngine