#pragma once

#include <ResourceManager.h>
#include <corona_logger.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <corona/interfaces/ISystem.h>
#include "SafeCommandQueue.h"
#include "SafeDataCache.h"
#include "EventBus.h"


namespace Corona {
// 全局数据ID生成器
struct DataId {
    using id_type = uint64_t;
    static id_type next();

   private:
    static std::atomic<id_type> counter_;
};

// 类型擦除的数据缓存中心，按类型提供 SafeDataCache<T>
class DataCacheHub {
   public:
    struct IHolder {
        virtual ~IHolder() = default;
    };
    template <typename T>
    struct Holder : IHolder {
        SafeDataCache<T> cache;
    };

    template <typename T>
    SafeDataCache<T>& get() {
        const std::type_index key{typeid(T)};
        {
            std::shared_lock lock(mutex_);
            if (auto iter = caches_.find(key); iter != caches_.end()) {
                return static_cast<Holder<T>&>(*iter->second).cache;
            }
        }
        std::unique_lock lock(mutex_);
        if (auto iter = caches_.find(key); iter != caches_.end()) {
            return static_cast<Holder<T>&>(*iter->second).cache;
        }
        auto ptr = std::make_unique<Holder<T>>();
        auto raw = ptr.get();
        caches_.emplace(key, std::move(ptr));
        return raw->cache;
    }

   private:
    std::unordered_map<std::type_index, std::unique_ptr<IHolder>> caches_{};
    std::shared_mutex mutex_{};
};

// 类型擦除的事件总线中心，按负载类型提供 EventBusT<T>
class EventBusHub {
   public:
    struct IHolder { virtual ~IHolder() = default; };
    template <typename T>
    struct Holder : IHolder { EventBusT<T> bus; };

    template <typename T>
    EventBusT<T>& get() {
        const std::type_index key{typeid(T)};
        {
            std::shared_lock lock(mutex_);
            if (auto it = buses_.find(key); it != buses_.end()) {
                return static_cast<Holder<T>&>(*it->second).bus;
            }
        }
        std::unique_lock lock(mutex_);
        if (auto it = buses_.find(key); it != buses_.end()) {
            return static_cast<Holder<T>&>(*it->second).bus;
        }
        auto ptr = std::make_unique<Holder<T>>();
        auto raw = ptr.get();
        buses_.emplace(key, std::move(ptr));
        return raw->bus;
    }

   private:
    std::unordered_map<std::type_index, std::unique_ptr<IHolder>> buses_{};
    std::shared_mutex mutex_{};
};

class Engine {
   public:
    static Engine& instance();

    // 初始化/收尾：配置日志并注册默认资源加载器
    void init(const LogConfig& cfg);
    void shutdown();

    // 访问器
    ResourceManager& resources();

    // 系统管理（基于 ISystem）
    template <typename T>
        requires std::is_base_of_v<ISystem, T>
    void register_system() {
        const std::type_index key{typeid(T)};
        if (systems_.contains(key)) {
            return;
        }
        systems_.emplace(key, std::make_shared<T>());
        CE_LOG_DEBUG("Registered system {}", key.name());
    }

    template <typename T>
        requires std::is_base_of_v<ISystem, T>
    T& get_system() const {
        const std::type_index key{typeid(T)};
        if (auto iter = systems_.find(key); iter != systems_.end()) {
            return *std::static_pointer_cast<T>(iter->second);
        }
        throw std::runtime_error(std::string("System not registered: ") + key.name());
    }

    void start_systems();
    void stop_systems();

    // 命令队列（跨线程命令分发）
    SafeCommandQueue& get_queue(const std::string& name) const;
    void add_queue(const std::string& name, std::unique_ptr<SafeCommandQueue> queue);

    // 全局数据缓存中心
    template <typename T>
    SafeDataCache<T>& cache() {
        return data_hub_.get<T>();
    }

    // 全局事件总线中心
    template <typename T>
    EventBusT<T>& events() {
        return event_hub_.get<T>();
    }

   private:
    Engine() = default;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

   private:
    bool initialized_ = false;
    std::unique_ptr<ResourceManager> resource_manager_{};

    // 新增：系统、队列、数据缓存
    std::unordered_map<std::type_index, std::shared_ptr<ISystem>> systems_{};
    std::unordered_map<std::string, std::unique_ptr<SafeCommandQueue>> queues_{};
    DataCacheHub data_hub_{};
    EventBusHub event_hub_{};
};
}  // namespace Corona
