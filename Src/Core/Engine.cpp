//
// Created by 47226 on 2025/9/8.
//

#include "Engine.h"

#include "Multimedia/Animation/AnimationSystemDefault.hpp"
#include "Multimedia/Audio/AudioSystemDefault.hpp"
#include "Multimedia/Display/DisplaySystemDefault.hpp"
#include "Multimedia/Rendering/RenderingSystemDefault.hpp"

std::atomic<Corona::DataCache::id_type> Corona::DataCache::id_counter = 0;

namespace Corona
{
    Engine::Engine()
        : engineLogger(std::make_shared<Logger>())
    {
    }
    Engine::~Engine()
    {
        LOG_DEBUG("Engine destroyed");
        for (const auto system : systems | std::views::values)
        {
            system->stop();
        }
    }
    DataCache::id_type DataCache::get_next_id()
    {
        return id_counter.fetch_add(1);
    }

    Logger &Engine::logger() const
    {
        return *engineLogger;
    }

    Engine &Engine::inst()
    {
        static Engine engine;
        return engine;
    }
    void Engine::init()
    {
        inst().register_system<AnimationSystemDefault>();
        inst().register_system<RenderingSystemDefault>();
        inst().register_system<AudioSystemDefault>();
        inst().register_system<DisplaySystemDefault>();
        for (const auto system : systems | std::views::values)
        {
            system->start();
        }
    }
    DataCache &Engine::data_cache()
    {
        return this->data;
    }
    const DataCache &Engine::data_cache() const
    {
        return this->data;
    }
} // namespace Corona