#include "ResourceManager.h"
#include <Log.h>

#include <algorithm>
#include <mutex>

namespace Corona
{
    ResourceManager::ResourceManager() = default;
    ResourceManager::~ResourceManager()
    {
        tasks_.wait();
    }

    void ResourceManager::registerLoader(std::shared_ptr<IResourceLoader> loader)
    {
        if (!loader)
            return;
        std::unique_lock lk(loadersMutex_);
        loaders_.push_back(std::move(loader));
    }

    void ResourceManager::unregisterLoader(std::shared_ptr<IResourceLoader> loader)
    {
        std::unique_lock lk(loadersMutex_);
        loaders_.erase(std::remove(loaders_.begin(), loaders_.end(), loader), loaders_.end());
    }

    std::shared_ptr<IResource> ResourceManager::loadInternal(const ResourceId &id)
    {
        if (auto it = cache_.find(id); it != cache_.end())
            return it->second;

        std::shared_ptr<std::mutex> mtx;
        if (auto it = locks_.find(id); it != locks_.end())
        {
            mtx = it->second;
        }
        else
        {
            auto created = std::make_shared<std::mutex>();
            auto [insIt, ok] = locks_.insert({id, created});
            mtx = insIt->second;
        }

        std::scoped_lock lk(*mtx);
        if (auto it2 = cache_.find(id); it2 != cache_.end())
            return it2->second;

        auto loader = findLoader(id);
        if (!loader)
        {
            CE_LOG_ERROR("No loader for type='{}' path='{}'", id.type, id.path);
            return nullptr;
        }

        auto res = loader->load(id);
        if (!res)
        {
            CE_LOG_ERROR("Load failed for type='{}' path='{}'", id.type, id.path);
            return nullptr;
        }

        cache_.insert({id, res});
        return res;
    }

    std::shared_ptr<IResourceLoader> ResourceManager::findLoader(const ResourceId &id)
    {
        std::shared_lock lk(loadersMutex_);
        for (auto &l : loaders_)
        {
            if (l && l->supports(id))
                return l;
        }
        return nullptr;
    }

    std::shared_ptr<IResource> ResourceManager::load(const ResourceId &id)
    {
        return loadInternal(id);
    }

    std::shared_ptr<IResource> ResourceManager::loadOnce(const ResourceId &id)
    {
        auto loader = findLoader(id);
        if (!loader)
        {
            CE_LOG_ERROR("No loader for type='{}' path='{}' (loadOnce)", id.type, id.path);
            return nullptr;
        }
        auto res = loader->load(id);
        if (!res)
        {
            CE_LOG_ERROR("LoadOnce failed for type='{}' path='{}'", id.type, id.path);
        }
        return res;
    }

    std::future<std::shared_ptr<IResource>> ResourceManager::loadAsync(const ResourceId &id)
    {
        auto prom = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
        auto fut = prom->get_future();
        tasks_.run([this, id, prom] {
            prom->set_value(this->loadInternal(id));
        });
        return fut;
    }

    std::future<std::shared_ptr<IResource>> ResourceManager::loadOnceAsync(const ResourceId &id)
    {
        auto prom = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
        auto fut = prom->get_future();
        tasks_.run([this, id, prom] {
            prom->set_value(this->loadOnce(id));
        });
        return fut;
    }

    void ResourceManager::loadAsync(const ResourceId &id, LoadCallback cb)
    {
        tasks_.run([this, id, cb] {
            auto res = loadInternal(id);
            if (cb)
                cb(id, res);
        });
    }

    void ResourceManager::loadOnceAsync(const ResourceId &id, LoadCallback cb)
    {
        tasks_.run([this, id, cb] {
            auto res = loadOnce(id);
            if (cb)
                cb(id, res);
        });
    }

    void ResourceManager::preload(const std::vector<ResourceId> &ids)
    {
        for (const auto &rid : ids)
        {
            tasks_.run([this, rid]() {
                (void)loadInternal(rid);
            });
        }
    }

    void ResourceManager::wait()
    {
        tasks_.wait();
    }

    void ResourceManager::clear()
    {
        cache_.clear();
    }

    bool ResourceManager::contains(const ResourceId &id) const
    {
        return cache_.find(id) != cache_.end();
    }
} // namespace Corona
