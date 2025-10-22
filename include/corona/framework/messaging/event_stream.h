#pragma once

#include <chrono>
#include <cstddef>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace corona::framework::messaging {

enum class backpressure_policy {
    block,
    drop_oldest,
    drop_newest
};

struct event_stream_options {
    std::size_t max_queue_size = 256;
    backpressure_policy policy = backpressure_policy::block;
};

template <typename T>
class event_stream;

template <typename T>
class event_subscription {
   public:
    event_subscription() = default;
    event_subscription(event_subscription&& other) noexcept = default;
    event_subscription& operator=(event_subscription&& other) noexcept = default;
    ~event_subscription();

    event_subscription(const event_subscription&) = delete;
    event_subscription& operator=(const event_subscription&) = delete;

    bool valid() const;
    bool try_pop(T& out);
    bool wait_for(T& out, std::chrono::milliseconds timeout);
    void close();

   private:
    friend class event_stream<T>;

    struct state;

    event_subscription(std::shared_ptr<state> state, event_stream<T>* owner);

    void release();

    std::shared_ptr<state> state_{};
    event_stream<T>* owner_ = nullptr;
};

template <typename T>
class event_stream {
   public:
    using value_type = T;

    event_stream() = default;
    ~event_stream() = default;

    event_stream(const event_stream&) = delete;
    event_stream& operator=(const event_stream&) = delete;

    event_subscription<T> subscribe(event_stream_options options = {});
    void publish(const T& payload);
    void publish(T&& payload);
    void unsubscribe(std::size_t id);

   private:
    friend class event_subscription<T>;

    struct subscriber_state {
        std::size_t id;
        event_stream_options options;
        std::deque<T> queue;
        std::mutex mutex;
        std::condition_variable condition;
        bool closed = false;
    };

    std::shared_ptr<typename event_subscription<T>::state> make_state(std::shared_ptr<subscriber_state> sub);

    void publish_impl(const T& payload);

    std::mutex subscribers_mutex_;
    std::unordered_map<std::size_t, std::shared_ptr<subscriber_state>> subscribers_;
    std::size_t next_id_ = 1;
};

template <typename T>
struct event_subscription<T>::state {
    std::shared_ptr<typename event_stream<T>::subscriber_state> subscriber;
};

template <typename T>
event_subscription<T>::event_subscription(std::shared_ptr<state> state, event_stream<T>* owner)
    : state_(std::move(state)), owner_(owner) {}

template <typename T>
event_subscription<T>::~event_subscription() {
    release();
}

template <typename T>
bool event_subscription<T>::valid() const {
    return state_ && state_->subscriber;
}

template <typename T>
void event_subscription<T>::close() {
    release();
}

template <typename T>
void event_subscription<T>::release() {
    if (!state_) {
        return;
    }
    auto sub = state_->subscriber;
    state_.reset();
    if (!sub) {
        return;
    }
    std::unique_lock<std::mutex> lock(sub->mutex);
    sub->closed = true;
    sub->queue.clear();
    lock.unlock();
    sub->condition.notify_all();
    if (owner_) {
        owner_->unsubscribe(sub->id);
    }
}

template <typename T>
bool event_subscription<T>::try_pop(T& out) {
    if (!state_ || !state_->subscriber) {
        return false;
    }
    auto sub = state_->subscriber;
    std::lock_guard<std::mutex> lock(sub->mutex);
    if (sub->queue.empty()) {
        return false;
    }
    out = std::move(sub->queue.front());
    sub->queue.pop_front();
    sub->condition.notify_all();
    return true;
}

template <typename T>
bool event_subscription<T>::wait_for(T& out, std::chrono::milliseconds timeout) {
    if (!state_ || !state_->subscriber) {
        return false;
    }
    auto sub = state_->subscriber;
    std::unique_lock<std::mutex> lock(sub->mutex);
    if (!sub->condition.wait_for(lock, timeout, [&] { return sub->closed || !sub->queue.empty(); })) {
        return false;
    }
    if (sub->closed || sub->queue.empty()) {
        return false;
    }
    out = std::move(sub->queue.front());
    sub->queue.pop_front();
    sub->condition.notify_all();
    return true;
}

template <typename T>
event_subscription<T> event_stream<T>::subscribe(event_stream_options options) {
    auto subscriber = std::make_shared<subscriber_state>();
    subscriber->id = next_id_++;
    if (options.max_queue_size == 0) {
        options.max_queue_size = 1;
    }
    subscriber->options = options;

    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        subscribers_.emplace(subscriber->id, subscriber);
    }

    return event_subscription<T>(make_state(subscriber), this);
}

template <typename T>
void event_stream<T>::publish(const T& payload) {
    publish_impl(payload);
}

template <typename T>
void event_stream<T>::publish(T&& payload) {
    publish_impl(payload);
}

template <typename T>
void event_stream<T>::publish_impl(const T& payload) {
    std::vector<std::shared_ptr<subscriber_state>> snapshot;
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        snapshot.reserve(subscribers_.size());
        for (auto const& entry : subscribers_) {
            snapshot.push_back(entry.second);
        }
    }

    for (auto& sub : snapshot) {
        std::unique_lock<std::mutex> lock(sub->mutex);
        if (sub->closed) {
            continue;
        }
        auto const max_size = sub->options.max_queue_size;
        switch (sub->options.policy) {
            case backpressure_policy::block: {
                sub->condition.wait(lock, [&] { return sub->closed || sub->queue.size() < max_size; });
                if (sub->closed) {
                    continue;
                }
                sub->queue.push_back(payload);
                break;
            }
            case backpressure_policy::drop_oldest: {
                if (sub->queue.size() >= max_size) {
                    if (!sub->queue.empty()) {
                        sub->queue.pop_front();
                    }
                }
                sub->queue.push_back(payload);
                break;
            }
            case backpressure_policy::drop_newest: {
                if (sub->queue.size() >= max_size) {
                    break;
                }
                sub->queue.push_back(payload);
                break;
            }
        }
        lock.unlock();
        sub->condition.notify_all();
    }
}

template <typename T>
void event_stream<T>::unsubscribe(std::size_t id) {
    std::shared_ptr<subscriber_state> removed;
    {
        std::lock_guard<std::mutex> lock(subscribers_mutex_);
        auto it = subscribers_.find(id);
        if (it != subscribers_.end()) {
            removed = it->second;
            subscribers_.erase(it);
        }
    }
    if (removed) {
        std::lock_guard<std::mutex> guard(removed->mutex);
        removed->closed = true;
        removed->queue.clear();
        removed->condition.notify_all();
    }
}

template <typename T>
std::shared_ptr<typename event_subscription<T>::state> event_stream<T>::make_state(std::shared_ptr<subscriber_state> sub) {
    auto state = std::make_shared<typename event_subscription<T>::state>();
    state->subscriber = std::move(sub);
    return state;
}

}  // namespace corona::framework::messaging
