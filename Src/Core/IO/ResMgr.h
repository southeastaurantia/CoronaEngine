//
// Created by 47226 on 2025/9/8.
//

#ifndef CORONAENGINE_RESOURCECACHE_H
#define CORONAENGINE_RESOURCECACHE_H
#include "Core/Logger.h"
#include "Engine.h"
#include "ResourceLoader.h"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/task_group.h"

#include <string>
#include <typeindex>

namespace Corona
{

    template <typename TRes>
        requires std::default_initializable<TRes> && std::is_base_of_v<Corona::Resource, TRes>
    class ResMgr
    {
      public:
        using Cache = tbb::concurrent_hash_map<std::string, typename ResourceLoader<TRes>::Handle>;

        template <typename TLoader>
            requires std::is_base_of_v<ResourceLoader<TRes>, TLoader> && std::default_initializable<TLoader>
        static void register_loader()
        {
            auto loader_type = std::type_index(typeid(TLoader)).name();
            std::lock_guard lock(loader_mutex);
            if (loader != nullptr)
            {
                LOG_WARN("Resource loader '{}' already registered", loader_type);
                return;
            }
            loader = std::make_unique<TLoader>();
            LOG_DEBUG("Register resource loader '{}'", loader_type);
        }

        static ResourceLoader<TRes>::Handle load(const std::string &path)
        {
            if (typename Cache::accessor accessor;
                cache_res.find(accessor, path) &&
                (accessor->second->get_status() == Resource::Status::OK ||
                 accessor->second->get_status() == Resource::Status::LOADING))
            {
                LOG_DEBUG("Load cache resource '{}'", path);
                return accessor->second;
            }

            if (loader == nullptr)
            {
                LOG_ERROR("Resource type '{}' not registered a loader", std::type_index(typeid(TRes)).name());
                return nullptr;
            }

            typename ResourceLoader<TRes>::Handle res = std::make_shared<TRes>();
            res->set_status(Resource::Status::LOADING);
            auto is_inserted = cache_res.emplace(path, res);

            // 如果插入失败，说明在第一次检查后，有另一个线程插入了占位符
            if (!is_inserted)
            {
                LOG_DEBUG("Load cache2 resource '{}'", path);
                // 再次查找以获取已存在的句柄
                if (typename Cache::accessor accessor; cache_res.find(accessor, path))
                {
                    return accessor->second;
                }
                // 这是一个理论上可能但极少发生的竞争条件：
                // emplace 失败后，资源马上被另一个线程移除了。
                // 在这种情况下，返回空指针是比较安全的处理方式。
                LOG_ERROR("Inconsistent cache state for resource '{}'", path);
                return nullptr;
            }

            tasks.run([&] {
                if (loader->load(path, res))
                {
                    res->set_status(Resource::Status::OK);
                }
            });

            tasks.wait();

            if (res == nullptr)
            {
                LOG_ERROR("Load resource '{}' failed, get null handle", path);
                cache_res.erase(path);
                return nullptr;
            }

            LOG_DEBUG("Load resource '{}' ok", path);
            return res;
        }

      private:
        static Cache cache_res;
        static std::unique_ptr<ResourceLoader<TRes>> loader;
        static std::mutex loader_mutex;
        static tbb::task_group tasks;
    };

} // namespace Corona

template <typename TRes>
    requires std::default_initializable<TRes> && std::is_base_of_v<Corona::Resource, TRes>
Corona::ResMgr<TRes>::Cache Corona::ResMgr<TRes>::cache_res = {};

template <typename TRes>
    requires std::default_initializable<TRes> && std::is_base_of_v<Corona::Resource, TRes>
std::unique_ptr<Corona::ResourceLoader<TRes>> Corona::ResMgr<TRes>::loader = nullptr;

template <typename TRes>
    requires std::default_initializable<TRes> && std::is_base_of_v<Corona::Resource, TRes>
std::mutex Corona::ResMgr<TRes>::loader_mutex = {};

template <typename TRes>
    requires std::default_initializable<TRes> && std::is_base_of_v<Corona::Resource, TRes>
tbb::task_group Corona::ResMgr<TRes>::tasks = {};

#endif // CORONAENGINE_RESOURCECACHE_H
