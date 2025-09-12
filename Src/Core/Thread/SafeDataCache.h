//
// Created by 47226 on 2025/9/9.
//

#pragma once

#include "Core/Engine.h"
#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstdint>
#include <queue>
#include <typeindex>
#include <unordered_set>

namespace Corona
{
    template <typename TData>
    class SafeDataCache final
    {
      public:
        using id_type = uint64_t;
        using mutexes_type = tbb::concurrent_hash_map<id_type, std::shared_ptr<std::mutex>>;
        using caches_type = tbb::concurrent_hash_map<id_type, std::shared_ptr<TData>>;

        auto size() const
        {
            return data_cache.size();
        }

        bool insert(const id_type &id, std::shared_ptr<TData> data)
        {
            if (!data_cache.emplace(id, data))
            {
                LOG_WARN("Data id:{} type:{} insert failed", id, std::type_index(typeid(TData)).name());
                return false;
            }
            foreach_mutex.emplace(id, std::make_shared<std::mutex>());
            return true;
        }

        bool erase(const id_type &id)
        {
            if (!data_cache.erase(id))
            {
                LOG_WARN("Data id:{} type:{} erase failed", id, std::type_index(typeid(TData)).name());
                return false;
            }
            foreach_mutex.erase(id);
            return true;
        }

        std::shared_ptr<const TData> get(const id_type &id) const
        {
            typename caches_type::accessor data_it;
            if (!data_cache.find(data_it, id))
            {
                LOG_WARN("Data id:{} type:{} get failed", id, std::type_index(typeid(TData)).name());
                return nullptr;
            }
            return data_it->second;
        }

        bool modify(const id_type &id, std::function<void(std::shared_ptr<TData>)> callback)
        {
            typename caches_type::accessor data_it;
            mutexes_type::accessor foreach_it;

            if (!data_cache.find(data_it, id))
            {
                LOG_WARN("Data id:{} type:{} modify failed, data not found", id, std::type_index(typeid(TData)).name());
                return false;
            }
            if (!foreach_mutex.find(foreach_it, id))
            {
                LOG_WARN("Data id:{} type:{} modify failed, mutex not found", id, std::type_index(typeid(TData)).name());
                return false;
            }
            std::lock_guard lock(*foreach_it->second);
            callback(data_it->second);
            return true;
        }

        void safe_loop_foreach(const std::unordered_set<id_type> &data_ids, std::function<void(std::shared_ptr<TData>)> callback)
        {
            std::queue<id_type> unhandled_data_ids;
            for (auto const &id : data_ids)
            {
                typename caches_type::accessor data_it;
                mutexes_type::accessor foreach_it;
                if (!data_cache.find(data_it, id))
                {
                    continue;
                }
                if (!foreach_mutex.find(foreach_it, id))
                {
                    continue;
                }
                if (foreach_it->second->try_lock())
                {
                    callback(data_it->second);
                    foreach_it->second->unlock();
                    continue;
                }
                unhandled_data_ids.push(id);
            }

            while (!unhandled_data_ids.empty())
            {
                auto const &id = unhandled_data_ids.front();
                unhandled_data_ids.pop();
                typename caches_type::accessor data_it;
                mutexes_type::accessor foreach_it;
                if (!data_cache.find(data_it, id))
                {
                    continue;
                }
                if (!foreach_mutex.find(foreach_it, id))
                {
                    continue;
                }
                if (foreach_it->second->try_lock())
                {
                    callback(data_it->second);
                    foreach_it->second->unlock();
                    continue;
                }
                unhandled_data_ids.push(id);
            }
        }

      private:
        caches_type data_cache{};
        mutexes_type foreach_mutex{};
    };
} // namespace Corona
