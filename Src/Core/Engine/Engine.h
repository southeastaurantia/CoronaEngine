#pragma once

#include "Core/Engine/ISystem.h"
#include "Core/IO/ResourceManager.h"
#include "Core/Log.h"
#include "Core/Thread/SafeCommandQueue.h"
#include "Core/Thread/SafeDataCache.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <typeindex>
#include <unordered_map>

namespace Corona
{
    // 全局数据ID生成器
    struct DataId
    {
        using id_type = uint64_t;
        static id_type Next();

      private:
        static std::atomic<id_type> counter_;
    };

    // 类型擦除的数据缓存中心，按类型提供 SafeDataCache<T>
    class DataCacheHub
    {
      public:
        struct IHolder
        {
            virtual ~IHolder() = default;
        };
        template <typename T>
        struct Holder : IHolder
        {
            SafeDataCache<T> cache;
        };

        template <typename T>
        SafeDataCache<T> &Get()
        {
            const std::type_index key{typeid(T)};
            {
                std::shared_lock rlk(mtx_);
                if (auto it = caches_.find(key); it != caches_.end())
                {
                    return static_cast<Holder<T> &>(*it->second).cache;
                }
            }
            std::unique_lock wlk(mtx_);
            if (auto it = caches_.find(key); it != caches_.end())
            {
                return static_cast<Holder<T> &>(*it->second).cache;
            }
            auto ptr = std::make_unique<Holder<T>>();
            auto raw = ptr.get();
            caches_.emplace(key, std::move(ptr));
            return raw->cache;
        }

      private:
        std::unordered_map<std::type_index, std::unique_ptr<IHolder>> caches_{};
        std::shared_mutex mtx_{};
    };

    class Engine
    {
      public:
        static Engine &Instance();

        // 初始化/收尾：配置日志并注册默认资源加载器
        void Init(const LogConfig &cfg);
        void Shutdown();

        // 访问器
        ResourceManager &Resources();

        // 系统管理（基于 ISystem）
        template <typename T>
            requires std::is_base_of_v<ISystem, T>
        void RegisterSystem()
        {
            const std::type_index key{typeid(T)};
            if (systems_.contains(key))
                return;
            systems_.emplace(key, std::make_shared<T>());
            CE_LOG_DEBUG("Registered system {}", key.name());
        }

        template <typename T>
            requires std::is_base_of_v<ISystem, T>
        T &GetSystem() const
        {
            const std::type_index key{typeid(T)};
            if (auto it = systems_.find(key); it != systems_.end())
            {
                return *std::static_pointer_cast<T>(it->second);
            }
            throw std::runtime_error(std::string("System not registered: ") + key.name());
        }

        void StartSystems();
        void StopSystems();

        // 命令队列（跨线程命令分发）
        SafeCommandQueue &GetQueue(const std::string &name) const;
        void AddQueue(const std::string &name, std::unique_ptr<SafeCommandQueue> q);

        // 全局数据缓存中心
        template <typename T>
        SafeDataCache<T> &Cache()
        {
            return dataHub_.Get<T>();
        }

      private:
        Engine() = default;
        ~Engine() = default;
        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;

      private:
        bool inited_ = false;
        std::unique_ptr<ResourceManager> resMgr_{};

        // 新增：系统、队列、数据缓存
        std::unordered_map<std::type_index, std::shared_ptr<ISystem>> systems_{};
        std::unordered_map<std::string, std::unique_ptr<SafeCommandQueue>> queues_{};
        DataCacheHub dataHub_{};
    };
} // namespace Corona
