#include "Engine.h"
#include "Core/IO/Loaders/BinaryResource.h"
#include "Core/IO/Loaders/TextResource.h"
#include "Resource/Model.h"
#include "Resource/Shader.h"

#include <ranges>

namespace Corona
{
    Engine &Engine::Instance()
    {
        static Engine inst;
        return inst;
    }

    // --- DataId ---
    std::atomic<DataId::id_type> DataId::counter_ = 0;
    DataId::id_type DataId::Next()
    {
        return counter_.fetch_add(1, std::memory_order_relaxed);
    }

    void Engine::Init(const LogConfig &cfg)
    {
        if (inited_)
            return;

        // Init logging
        Logger::Init(cfg);

        // Create ResourceManager and register default loaders
        resMgr_ = std::make_unique<ResourceManager>();
        resMgr_->registerLoader(std::make_shared<TextResourceLoader>());
        resMgr_->registerLoader(std::make_shared<BinaryResourceLoader>());
        // Register engine resource loaders
        resMgr_->registerLoader(std::make_shared<ModelLoader>());
        resMgr_->registerLoader(std::make_shared<ShaderLoader>());

        inited_ = true;
    }

    void Engine::Shutdown()
    {
        if (!inited_)
            return;

        // 停止并清理系统
        StopSystems();
        systems_.clear();
        queues_.clear();

        if (resMgr_)
        {
            resMgr_->wait();
            resMgr_->clear();
            resMgr_.reset();
        }

        Logger::Shutdown();
        inited_ = false;
    }

    ResourceManager &Engine::Resources()
    {
        return *resMgr_;
    }

    void Engine::StartSystems()
    {
        for (const auto &sys : systems_ | std::views::values)
        {
            sys->start();
        }
    }

    void Engine::StopSystems()
    {
        for (const auto &sys : systems_ | std::views::values)
        {
            sys->stop();
        }
    }

    SafeCommandQueue &Engine::GetQueue(const std::string &name) const
    {
        if (auto it = queues_.find(name); it != queues_.end())
        {
            return *it->second;
        }
        throw std::runtime_error("Unknown command queue: " + name);
    }

    void Engine::AddQueue(const std::string &name, std::unique_ptr<SafeCommandQueue> q)
    {
        if (!q)
        {
            CE_LOG_ERROR("Cannot add null command queue for '{}'", name);
            return;
        }
        if (queues_.contains(name))
        {
            CE_LOG_WARN("Cannot add command queue for '{}' twice", name);
            return;
        }
        queues_[name] = std::move(q);
        CE_LOG_DEBUG("Added command queue for '{}'", name);
    }
} // namespace Corona
