#pragma once

#include <cabbage_concurrent/container/mpmc_queue.h>
#include <cabbage_concurrent/container/concurrent_hash_map.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Corona {

// A simple, thread-safe pub/sub built on cabbageconcurrent.
// - Per topic, registered subscribers receive a copy of the published payload.
// - Not subscribed => no delivery.
// - Each subscriber gets its own MPMC queue to poll in its own thread.
// - Subscribe/unsubscribe are O(1) average with coarse per-topic mutex for iteration safety.

template <typename TPayload>
class EventBusT {
   public:
    using Topic = std::string;
    using SubscriberId = std::uint64_t;

    // Single-subscriber queue wrapper
    class Queue {
       public:
        using value_type = TPayload;
        bool try_pop(value_type& out) { return q_.try_dequeue(out); }
        void push(const value_type& v) { q_.enqueue(v); }
        void push(value_type&& v) { q_.enqueue(std::move(v)); }
        bool empty() const { return q_.empty(); }
       private:
        Cabbage::Concurrent::MPMCQueue<value_type> q_;
    };

    struct Subscription {
        Topic topic;
        SubscriberId id = 0;
        std::shared_ptr<Queue> queue; // poll this in your system thread
    };

    // Subscribe to a topic; returns a Subscription with a queue to poll.
    Subscription subscribe(const Topic& topic) {
        auto subs = get_or_create_topic(topic);
        auto q = std::make_shared<Queue>();
        const auto id = next_id_.fetch_add(1, std::memory_order_relaxed) + 1; // start from 1
        {
            std::lock_guard lk(subs->mtx);
            subs->subscribers.emplace(id, q);
        }
        return Subscription{topic, id, std::move(q)};
    }

    // Unsubscribe by id from a topic. Safe to call multiple times.
    void unsubscribe(const Topic& topic, SubscriberId id) {
        auto subs = topics_.find(topic);
        if (!subs) return;
        auto& s = *subs;
        std::lock_guard lk(s->mtx);
        s->subscribers.erase(id);
    }

    // Publish copies to all current subscribers of the topic.
    void publish(const Topic& topic, const TPayload& payload) {
        auto subs = topics_.find(topic);
        if (!subs) return;
        // Copy out weak list under lock, then deliver without holding the lock
        std::vector<std::shared_ptr<Queue>> queues;
        {
            auto& s = *subs;
            std::lock_guard lk(s->mtx);
            queues.reserve(s->subscribers.size());
            for (auto& [_, q] : s->subscribers) {
                queues.push_back(q);
            }
        }
        for (auto& q : queues) {
            if (q) q->push(payload);
        }
    }

    void publish(const Topic& topic, TPayload&& payload) {
        auto subs = topics_.find(topic);
        if (!subs) return;
        std::vector<std::shared_ptr<Queue>> queues;
        {
            auto& s = *subs;
            std::lock_guard lk(s->mtx);
            queues.reserve(s->subscribers.size());
            for (auto& [_, q] : s->subscribers) {
                queues.push_back(q);
            }
        }
        for (auto& q : queues) {
            if (q) q->push(payload); // copy per-subscriber; preserves move-origin content for later reuse
        }
    }

    // Clear all topics and subscribers
    void clear() {
        // topics_ is concurrent; replace with empty by clearing each entry
        // Iterate keys by snapshot: we cannot iterate ConcurrentHashMap safely without API; rely on clear() in place.
        topics_.clear();
    }

   private:
    struct Subscribers {
        std::mutex mtx;
        std::unordered_map<SubscriberId, std::shared_ptr<Queue>> subscribers;
    };

    std::shared_ptr<Subscribers> get_or_create_topic(const Topic& topic) {
        if (auto existing = topics_.find(topic)) {
            return *existing;
        }
        auto created = std::make_shared<Subscribers>();
        if (topics_.insert(topic, created)) {
            return created;
        }
        // Lost race: find again
        auto retry = topics_.find(topic);
        return retry ? *retry : created;
    }

    Cabbage::Concurrent::ConcurrentHashMap<Topic, std::shared_ptr<Subscribers>> topics_{};
    std::atomic<SubscriberId> next_id_{0};
};

} // namespace Corona

