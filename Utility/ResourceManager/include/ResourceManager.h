#pragma once

// 迁移版 ResourceManager：从 Src/Core/IO 移至 Utility/ResourceManager
// 对外暴露统一头 <ResourceManager.h>

#include "IResource.h"
#include "IResourceLoader.h"
#include "ResourceTypes.h"

#include <oneapi/tbb/concurrent_unordered_map.h>
#include <oneapi/tbb/task_group.h>

#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

namespace Corona
{
    class ResourceManager
    {
      public:
        using LoadCallback = std::function<void(const ResourceId &, std::shared_ptr<IResource>)>; // nullptr 表示失败

        ResourceManager();
        ~ResourceManager();

        void registerLoader(std::shared_ptr<IResourceLoader> loader);
        void unregisterLoader(std::shared_ptr<IResourceLoader> loader);

        std::shared_ptr<IResource> load(const ResourceId &id);

        template <typename T>
        std::shared_ptr<T> loadTyped(const ResourceId &id)
        {
            auto r = load(id);
            return std::static_pointer_cast<T>(r);
        }

        std::future<std::shared_ptr<IResource>> loadAsync(const ResourceId &id);
        void loadAsync(const ResourceId &id, LoadCallback cb);

        std::shared_ptr<IResource> loadOnce(const ResourceId &id);
        std::future<std::shared_ptr<IResource>> loadOnceAsync(const ResourceId &id);
        void loadOnceAsync(const ResourceId &id, LoadCallback cb);

        void preload(const std::vector<ResourceId> &ids);
        void wait();

        void clear();
        bool contains(const ResourceId &id) const;

      private:
        std::shared_ptr<IResource> loadInternal(const ResourceId &id);
        std::shared_ptr<IResourceLoader> findLoader(const ResourceId &id);

      private:
        tbb::concurrent_unordered_map<ResourceId, std::shared_ptr<IResource>, ResourceIdHash> cache_;
        tbb::concurrent_unordered_map<ResourceId, std::shared_ptr<std::mutex>, ResourceIdHash> locks_;
        mutable std::shared_mutex loadersMutex_;
        std::vector<std::shared_ptr<IResourceLoader>> loaders_;
        tbb::task_group tasks_;
    };
} // namespace Corona
