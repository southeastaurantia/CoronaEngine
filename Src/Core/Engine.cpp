//
// Created by 47226 on 2025/9/8.
//

#include "Engine.h"

#include "Multimedia/Animation/AnimationSystemDefault.h"
#include "Multimedia/Audio/AudioSystemDefault.h"
#include "Multimedia/Display/DisplaySystemDefault.h"
#include "Multimedia/Rendering/RenderingSystemDefault.h"

#include <filesystem>

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
        for (const auto& system : systems | std::views::values)
        {
            system->stop();
        }
        resource_managers.clear();
        systems.clear();
        system_cmd_queues.clear();
        engineLogger.reset();
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
        for (const auto &system : systems | std::views::values)
        {
            system->start();
        }
    }
    SafeCommandQueue &Engine::get_cmd_queue(const std::string &name) const
    {
        if (const auto it = system_cmd_queues.find(name);
            it != system_cmd_queues.end())
        {
            return *it->second;
        }
        throw std::runtime_error("Unknown system command queue name");
    }
    void Engine::add_cmd_queue(const std::string &name, std::unique_ptr<SafeCommandQueue> cmd_queue)
    {
        if (!cmd_queue)
        {
            LOG_ERROR("Cannot add null command queue for '{}'", name);
            return;
        }
        if (system_cmd_queues.contains(name))
        {
            LOG_WARN("Cannot add command queue for '{}' twice", name);
            return;
        }
        system_cmd_queues[name] = std::move(cmd_queue);
        LOG_DEBUG("Added command queue for '{}'", name);
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