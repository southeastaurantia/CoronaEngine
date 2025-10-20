// 线程安全数据缓存：
// - 提供按 id 存取共享_ptr 数据，支持并发读写
// - 为每个 id 维护独立互斥，保障 foreach/tick 与外部修改的互斥
// - safe_loop_foreach 尝试无阻塞处理，无法立即加锁的 id 会排队重试
#pragma once

#include <cabbage_concurrent/container/concurrent_hash_map.h>
#include <corona_logger.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <typeindex>
#include <unordered_set>

namespace Corona {
template <typename TData>
class SafeDataCache final {
   public:
    using id_type = uint64_t;
    using mutexes_type = Cabbage::Concurrent::ConcurrentHashMap<id_type, std::shared_ptr<std::mutex>>;
    using caches_type = Cabbage::Concurrent::ConcurrentHashMap<id_type, std::shared_ptr<TData>>;

    // 当前缓存的元素个数
    auto size() const {
        return data_cache.size();
    }

    // 插入数据；若 id 已存在则返回 false 并记录日志
    bool insert(const id_type& id, std::shared_ptr<TData> data) {
        if (!data_cache.insert(id, data)) {
            CE_LOG_WARN("Data id:{} type:{} insert failed", id, std::type_index(typeid(TData)).name());
            return false;
        }
        if (!foreach_mutex.insert(id, std::make_shared<std::mutex>())) {
            CE_LOG_WARN("Data id:{} type:{} mutex insert failed", id, std::type_index(typeid(TData)).name());
            data_cache.erase(id);
            return false;
        }
        return true;
    }

    // 删除数据；若不存在返回 false 并记录日志
    bool erase(const id_type& id) {
        if (!data_cache.erase(id)) {
            CE_LOG_WARN("Data id:{} type:{} erase failed", id, std::type_index(typeid(TData)).name());
            return false;
        }
        foreach_mutex.erase(id);
        return true;
    }

    // 读取只读共享_ptr；失败返回 nullptr
    std::shared_ptr<const TData> get(const id_type& id) const {
        auto data = data_cache.find(id);
        if (!data) {
            CE_LOG_WARN("Data id:{} type:{} get failed", id, std::type_index(typeid(TData)).name());
            return nullptr;
        }
        return *data;
    }

    // 修改指定 id 的数据：
    // - 内部获取专用互斥，保障与 foreach 的互斥
    // - 回调在持锁状态下执行，请避免长耗时
    bool modify(const id_type& id, std::function<void(std::shared_ptr<TData>)> callback) {
        auto data = data_cache.find(id);
        if (!data) {
            CE_LOG_WARN("Data id:{} type:{} modify failed, data not found", id, std::type_index(typeid(TData)).name());
            return false;
        }
        auto mutex_ptr = foreach_mutex.find(id);
        if (!mutex_ptr) {
            CE_LOG_WARN("Data id:{} type:{} modify failed, mutex not found", id, std::type_index(typeid(TData)).name());
            return false;
        }
        auto mutex_shared = *mutex_ptr;
        if (!mutex_shared) {
            CE_LOG_WARN("Data id:{} type:{} modify failed, mutex null", id, std::type_index(typeid(TData)).name());
            return false;
        }
        std::lock_guard<std::mutex> lock(*mutex_shared);
        callback(*data);
        return true;
    }

    // 遍历一组 id 并对可立即加锁的项执行回调：
    // - 优先 try_lock 成功的项，失败的 id 放入队列稍后重试
    // - 回调不应抛异常/长时间阻塞
    void safe_loop_foreach(const std::unordered_set<id_type>& data_ids, std::function<void(std::shared_ptr<TData>)> callback) {
        std::queue<id_type> unhandled_data_ids;
        for (auto const& id : data_ids) {
            auto data = data_cache.find(id);
            if (!data) {
                continue;
            }
            auto mutex_ptr = foreach_mutex.find(id);
            if (!mutex_ptr) {
                continue;
            }
            auto mutex_handle = *mutex_ptr;
            if (!mutex_handle) {
                continue;
            }
            if (mutex_handle->try_lock()) {
                callback(*data);
                mutex_handle->unlock();
                continue;
            }
            unhandled_data_ids.push(id);
        }

        while (!unhandled_data_ids.empty()) {
            auto const& id = unhandled_data_ids.front();
            unhandled_data_ids.pop();
            auto data = data_cache.find(id);
            if (!data) {
                continue;
            }
            auto mutex_ptr = foreach_mutex.find(id);
            if (!mutex_ptr) {
                continue;
            }
            auto mutex_handle = *mutex_ptr;
            if (!mutex_handle) {
                continue;
            }
            if (mutex_handle->try_lock()) {
                callback(*data);
                mutex_handle->unlock();
                continue;
            }
            unhandled_data_ids.push(id);
        }
    }

    // 新增：支持回调接收 id 的重载版本
    void safe_loop_foreach(const std::unordered_set<id_type>& data_ids, std::function<void(id_type, std::shared_ptr<TData>)> callback) {
        std::queue<id_type> unhandled_data_ids;
        for (auto const& id : data_ids) {
            auto data = data_cache.find(id);
            if (!data) {
                continue;
            }
            auto mutex_ptr = foreach_mutex.find(id);
            if (!mutex_ptr) {
                continue;
            }
            auto mutex_handle = *mutex_ptr;
            if (!mutex_handle) {
                continue;
            }
            if (mutex_handle->try_lock()) {
                callback(id, *data);
                mutex_handle->unlock();
                continue;
            }
            unhandled_data_ids.push(id);
        }

        while (!unhandled_data_ids.empty()) {
            auto const& id = unhandled_data_ids.front();
            unhandled_data_ids.pop();
            auto data = data_cache.find(id);
            if (!data) {
                continue;
            }
            auto mutex_ptr = foreach_mutex.find(id);
            if (!mutex_ptr) {
                continue;
            }
            auto mutex_handle = *mutex_ptr;
            if (!mutex_handle) {
                continue;
            }
            if (mutex_handle->try_lock()) {
                callback(id, *data);
                mutex_handle->unlock();
                continue;
            }
            unhandled_data_ids.push(id);
        }
    }

   private:
    caches_type data_cache{};
    mutexes_type foreach_mutex{};
};
}  // namespace Corona
