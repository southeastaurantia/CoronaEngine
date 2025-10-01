#include "ResourceManager.h"
#include <Log.h>

#include <algorithm>
#include <exception>
#include <mutex>

namespace Corona
{
    ResourceManager::ResourceManager() = default;
    ResourceManager::~ResourceManager()
    {
        wait();
        taskPool_.shutdown();
    }

    void ResourceManager::scheduleTask(std::function<void()> task)
    {
        pendingTasks_.fetch_add(1, std::memory_order_acq_rel);
        try
        {
            taskPool_.submit_detached([this, task = std::move(task)]() mutable {
                try
                {
                    if (task)
                    {
                        task();
                    }
                }
                catch (const std::exception &e)
                {
                    CE_LOG_ERROR("ResourceManager async task threw exception: {}", e.what());
                }
                catch (...)
                {
                    CE_LOG_ERROR("ResourceManager async task threw unknown exception");
                }

                if (pendingTasks_.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    std::lock_guard lk(waitMutex_);
                    waitCv_.notify_all();
                }
            });
        }
        catch (...)
        {
            pendingTasks_.fetch_sub(1, std::memory_order_acq_rel);
            throw;
        }
    }

    void ResourceManager::registerLoader(std::shared_ptr<IResourceLoader> loader)
    {
        if (!loader)
        {
            return;
        }
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
        if (auto cached = cache_.find(id))
        {
            return *cached;
        }

        std::shared_ptr<std::mutex> mtx;
        if (auto existingLock = locks_.find(id))
        {
            mtx = *existingLock;
        }
        else
        {
            auto created = std::make_shared<std::mutex>();
            if (locks_.insert(id, created))
            {
                mtx = std::move(created);
            }
            else
            {
                auto retryLock = locks_.find(id);
                mtx = retryLock ? *retryLock : created;
            }
        }

        std::scoped_lock lk(*mtx);
        if (auto cachedAfterLock = cache_.find(id))
        {
            return *cachedAfterLock;
        }

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

        cache_.insert(id, res);
        return res;
    }

    std::shared_ptr<IResourceLoader> ResourceManager::findLoader(const ResourceId &id)
    {
        std::shared_lock lk(loadersMutex_);
        for (auto &l : loaders_)
        {
            if (l && l->supports(id))
            {
                return l;
            }
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
        scheduleTask([this, id, prom] {
            prom->set_value(this->loadInternal(id));
        });
        return fut;
    }

    std::future<std::shared_ptr<IResource>> ResourceManager::loadOnceAsync(const ResourceId &id)
    {
        auto prom = std::make_shared<std::promise<std::shared_ptr<IResource>>>();
        auto fut = prom->get_future();
        scheduleTask([this, id, prom] {
            prom->set_value(this->loadOnce(id));
        });
        return fut;
    }

    void ResourceManager::loadAsync(const ResourceId &id, LoadCallback cb)
    {
        scheduleTask([this, id, cb] {
            auto res = loadInternal(id);
            if (cb)
            {
                cb(id, res);
            }
        });
    }

    void ResourceManager::loadOnceAsync(const ResourceId &id, LoadCallback cb)
    {
        scheduleTask([this, id, cb] {
            auto res = loadOnce(id);
            if (cb)
            {
                cb(id, res);
            }
        });
    }

    void ResourceManager::preload(const std::vector<ResourceId> &ids)
    {
        for (const auto &rid : ids)
        {
            scheduleTask([this, rid]() {
                (void)loadInternal(rid);
            });
        }
    }

    void ResourceManager::wait()
    {
        std::unique_lock lk(waitMutex_);
        waitCv_.wait(lk, [this]() { return pendingTasks_.load(std::memory_order_acquire) == 0; });
    }

    void ResourceManager::clear()
    {
        cache_.clear();
        locks_.clear();
    }

    bool ResourceManager::contains(const ResourceId &id) const
    {
        return cache_.find(id).has_value();
    }
} // namespace Corona
