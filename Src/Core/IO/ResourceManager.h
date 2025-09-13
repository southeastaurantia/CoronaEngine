#pragma once

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
    // 简要契约：
    // 输入：ResourceId/type/path，回调可选
    // 输出：shared_ptr<IResource> 或 typed 指针
    // 错误：返回空指针并记录日志
    class ResourceManager
    {
      public:
        using LoadCallback = std::function<void(const ResourceId &, std::shared_ptr<IResource>)>; // nullptr 表示失败

        ResourceManager();
        ~ResourceManager();

        // 注册/取消注册加载器
        void registerLoader(std::shared_ptr<IResourceLoader> loader);
        void unregisterLoader(std::shared_ptr<IResourceLoader> loader);

        // 同步加载（带缓存）
        std::shared_ptr<IResource> load(const ResourceId &id);

        template <typename T>
        std::shared_ptr<T> loadTyped(const ResourceId &id)
        {
            auto r = load(id);
            return std::static_pointer_cast<T>(r);
        }

        // 异步加载，返回 future 或使用回调
        std::future<std::shared_ptr<IResource>> loadAsync(const ResourceId &id);
        void loadAsync(const ResourceId &id, LoadCallback cb);

  // 一次性加载（不进入缓存）
  // 同步版本：直接通过匹配的 loader 读取并返回（失败返回 nullptr）。
  std::shared_ptr<IResource> loadOnce(const ResourceId &id);
  // 异步版本：不进入缓存。
  std::future<std::shared_ptr<IResource>> loadOnceAsync(const ResourceId &id);
  void loadOnceAsync(const ResourceId &id, LoadCallback cb);

        // 预加载：并行排队加载，尽量复用缓存
        void preload(const std::vector<ResourceId> &ids);

        // 等待所有排队任务完成
        void wait();

        // 缓存管理
        void clear();
        bool contains(const ResourceId &id) const;

      private:
        std::shared_ptr<IResource> loadInternal(const ResourceId &id);
        std::shared_ptr<IResourceLoader> findLoader(const ResourceId &id);

      private:
        // 缓存：ResourceId -> IResource
        tbb::concurrent_unordered_map<ResourceId, std::shared_ptr<IResource>, ResourceIdHash> cache_;

        // In-flight 锁：防止同一资源被并发重复加载
        tbb::concurrent_unordered_map<ResourceId, std::shared_ptr<std::mutex>, ResourceIdHash> locks_;

        // 加载器集合（读多写少，用 shared_mutex）
        mutable std::shared_mutex loadersMutex_;
        std::vector<std::shared_ptr<IResourceLoader>> loaders_;

        // 异步加载组
        tbb::task_group tasks_;
    };
} // namespace Corona
