#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace corona::framework::messaging {

template <typename T>
class projection_subscription;

template <typename T>
class data_projection {
   public:
    using watcher_type = std::function<void(const T&)>;

    data_projection();

    data_projection(const data_projection&) = delete;
    data_projection& operator=(const data_projection&) = delete;

    void set(const T& value);
    void set(T&& value);

    [[nodiscard]] T get_snapshot() const;

    projection_subscription<T> subscribe(watcher_type watcher);
    void unsubscribe(std::size_t id);

   private:
    friend class projection_subscription<T>;

    mutable std::shared_mutex mutex_;
    T snapshot_{};
    std::unordered_map<std::size_t, watcher_type> watchers_;
    std::atomic<std::size_t> next_id_{1};
};

template <typename T>
class projection_subscription {
   public:
    projection_subscription() = default;
    projection_subscription(projection_subscription&& other) noexcept;
    projection_subscription& operator=(projection_subscription&& other) noexcept;
    ~projection_subscription();

    projection_subscription(const projection_subscription&) = delete;
    projection_subscription& operator=(const projection_subscription&) = delete;

    void release();

   private:
    friend class data_projection<T>;

    projection_subscription(data_projection<T>* owner, std::size_t id);

    data_projection<T>* owner_ = nullptr;
    std::size_t id_ = 0;
};

template <typename T>
data_projection<T>::data_projection() = default;

template <typename T>
void data_projection<T>::set(const T& value) {
    std::vector<watcher_type> watchers;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        snapshot_ = value;
        watchers.reserve(watchers_.size());
        for (auto const& [_, watcher] : watchers_) {
            watchers.push_back(watcher);
        }
    }
    for (auto& watcher : watchers) {
        watcher(snapshot_);
    }
}

template <typename T>
void data_projection<T>::set(T&& value) {
    std::vector<watcher_type> watchers;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        snapshot_ = std::move(value);
        watchers.reserve(watchers_.size());
        for (auto const& [_, watcher] : watchers_) {
            watchers.push_back(watcher);
        }
    }
    for (auto& watcher : watchers) {
        watcher(snapshot_);
    }
}

template <typename T>
T data_projection<T>::get_snapshot() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return snapshot_;
}

template <typename T>
projection_subscription<T> data_projection<T>::subscribe(watcher_type watcher) {
    auto id = next_id_.fetch_add(1);
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        watchers_.emplace(id, std::move(watcher));
    }
    return projection_subscription<T>(this, id);
}

template <typename T>
void data_projection<T>::unsubscribe(std::size_t id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    watchers_.erase(id);
}

template <typename T>
projection_subscription<T>::projection_subscription(data_projection<T>* owner, std::size_t id)
    : owner_(owner), id_(id) {}

template <typename T>
projection_subscription<T>::projection_subscription(projection_subscription&& other) noexcept
    : owner_(other.owner_), id_(other.id_) {
    other.owner_ = nullptr;
    other.id_ = 0;
}

template <typename T>
projection_subscription<T>& projection_subscription<T>::operator=(projection_subscription&& other) noexcept {
    if (this != &other) {
        release();
        owner_ = other.owner_;
        id_ = other.id_;
        other.owner_ = nullptr;
        other.id_ = 0;
    }
    return *this;
}

template <typename T>
projection_subscription<T>::~projection_subscription() {
    release();
}

template <typename T>
void projection_subscription<T>::release() {
    if (owner_ && id_ != 0) {
        owner_->unsubscribe(id_);
        owner_ = nullptr;
        id_ = 0;
    }
}

}  // namespace corona::framework::messaging
