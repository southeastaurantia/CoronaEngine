#include "Engine.h"
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

    // --- DataId锛氱畝鍗曡嚜澧?ID 鐢熸垚鍣?---
    std::atomic<DataId::id_type> DataId::counter_ = 0;
    DataId::id_type DataId::Next()
    {
        return counter_.fetch_add(1, std::memory_order_relaxed);
    }

    void Engine::Init(const LogConfig &cfg)
    {
        if (inited_)
            return;

        // 鍒濆鍖栨棩蹇?
        Logger::Init(cfg);

        // 鍒涘缓璧勬簮绠＄悊鍣ㄥ苟娉ㄥ唽榛樿鍔犺浇鍣?
    resMgr_ = std::make_unique<ResourceManager>();
    resMgr_->register_loader(std::make_shared<ModelLoader>());
    resMgr_->register_loader(std::make_shared<ShaderLoader>());

        inited_ = true;
    }

    void Engine::Shutdown()
    {
        if (!inited_)
            return;

        // 鍋滄骞舵竻鐞嗙郴缁?
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
        CE_LOG_CRITICAL("Command queue not found: {}", name);
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
