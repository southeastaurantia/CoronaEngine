#include "Engine.h"
#include "Model.h"
#include "Shader.h"

#include <ranges>

namespace Corona
{
    Engine &Engine::instance()
    {
        static Engine inst;
        return inst;
    }

    // --- DataId锛氱畝鍗曡嚜澧?ID 鐢熸垚鍣?---
    std::atomic<DataId::id_type> DataId::counter_ = 0;
    DataId::id_type DataId::next()
    {
        return counter_.fetch_add(1, std::memory_order_relaxed);
    }

    void Engine::init(const LogConfig &cfg)
    {
        if (initialized_)
        {
            return;
        }

        // 鍒濆鍖栨棩蹇?
        Logger::init(cfg);

        // 鍒涘缓璧勬簮绠＄悊鍣ㄥ苟娉ㄥ唽榛樿鍔犺浇鍣?
        resource_manager_ = std::make_unique<ResourceManager>();
        resource_manager_->register_loader(std::make_shared<ModelLoader>());
        resource_manager_->register_loader(std::make_shared<ShaderLoader>());

        initialized_ = true;
    }

    void Engine::shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        // 鍋滄骞舵竻鐞嗙郴缁?
        stop_systems();
        systems_.clear();
        queues_.clear();

        if (resource_manager_)
        {
            resource_manager_->wait();
            resource_manager_->clear();
            resource_manager_.reset();
        }

        Logger::shutdown();
        initialized_ = false;
    }

    ResourceManager &Engine::resources()
    {
        return *resource_manager_;
    }

    void Engine::start_systems()
    {
        for (const auto &sys : systems_ | std::views::values)
        {
            sys->start();
        }
    }

    void Engine::stop_systems()
    {
        for (const auto &sys : systems_ | std::views::values)
        {
            sys->stop();
        }
    }

    SafeCommandQueue &Engine::get_queue(const std::string &name) const
    {
        if (auto it = queues_.find(name); it != queues_.end())
        {
            return *it->second;
        }
        CE_LOG_CRITICAL("Command queue not found: {}", name);
        throw std::runtime_error(std::string("Command queue not found: ") + name);
    }

    void Engine::add_queue(const std::string &name, std::unique_ptr<SafeCommandQueue> queue)
    {
        if (!queue)
        {
            CE_LOG_ERROR("Cannot add null command queue for '{}'", name);
            return;
        }
        if (queues_.contains(name))
        {
            CE_LOG_WARN("Cannot add command queue for '{}' twice", name);
            return;
        }
        queues_[name] = std::move(queue);
        CE_LOG_DEBUG("Added command queue for '{}'", name);
    }
} // namespace Corona
