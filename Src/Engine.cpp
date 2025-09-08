//
// Created by 47226 on 2025/9/8.
//

#include "Engine.h"

namespace Corona
{
    Engine::Engine()
    {
    }
    Engine::~Engine()
    {
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