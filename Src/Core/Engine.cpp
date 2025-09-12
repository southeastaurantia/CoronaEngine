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
    SafeCommandQueue* Engine::get_cmd_queue(const std::string& system_name) const
    {
        auto it = system_cmd_queues.find(system_name);
        if (it != system_cmd_queues.end())
        {
            return it->second.get();
        }
        LOG_WARN("Command queue for system '{}' not found", system_name);
        return nullptr;
    }
    void Engine::add_cmd_queue(const std::string& system_name, std::unique_ptr<SafeCommandQueue> queue)
    {
        if (!queue)
        {
            LOG_ERROR("Cannot add null command queue for system '{}'", system_name);
            return;
        }
        system_cmd_queues[system_name] = std::move(queue);
        LOG_DEBUG("Added command queue for system '{}'", system_name);
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