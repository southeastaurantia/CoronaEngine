//
// Created by 47226 on 2025/9/8.
//

#include "Engine.h"

std::atomic<Corona::DataCache::id_type> Corona::DataCache::id_counter = 0;

namespace Corona
{
    Engine::Engine()
    {
    }
    Engine::~Engine()
    {
    }

    DataCache::id_type DataCache::get_next_id()
    {
        return id_counter.fetch_add(1);
    }

    Engine &Engine::inst()
    {
        static Engine engine;
        return engine;
    }
    void Engine::init()
    {
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