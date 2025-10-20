#pragma once

#include <corona/interfaces/Services.h>

#include <ResourceManager.h>

#include <functional>
#include <limits>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Corona::Testing {

class MockLogger final : public Interfaces::ILogger {
  public:
    struct Entry {
        Level level;
        std::string message;
    };

    void log(Level level, std::string_view message) override {
        records_.push_back({level, std::string(message)});
    }

    [[nodiscard]] const std::vector<Entry>& history() const { return records_; }

    void clear() { records_.clear(); }

  private:
    std::vector<Entry> records_{};
};

class MockResourceService final : public Interfaces::IResourceService {
  public:
    ResourcePtr load(const Corona::ResourceId& id) override { return fetch(id); }

    ResourcePtr load_once(const Corona::ResourceId& id) override { return fetch(id); }

    void load_async(const Corona::ResourceId& id, LoadCallback cb) override {
        if (cb) {
            cb(id, fetch(id));
        }
    }

    void load_once_async(const Corona::ResourceId& id, LoadCallback cb) override {
        if (cb) {
            cb(id, fetch(id));
        }
    }

    void preload(const std::vector<Corona::ResourceId>& ids) override {
        for (const auto& id : ids) {
            fetch(id);
        }
    }

    void clear() override { resources_.clear(); }

    void set_resource(Corona::ResourceId id, ResourcePtr resource) {
        resources_[std::move(id)] = std::move(resource);
    }

    bool remove_resource(const Corona::ResourceId& id) {
        return resources_.erase(id) > 0;
    }

  private:
    ResourcePtr fetch(const Corona::ResourceId& id) {
        if (auto it = resources_.find(id); it != resources_.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::unordered_map<Corona::ResourceId, ResourcePtr, Corona::ResourceIdHash> resources_{};
};

class MockCommandQueue final : public Interfaces::ICommandQueue {
  public:
    void enqueue(std::function<void()> command) override {
        if (command) {
            queue_.push(std::move(command));
        }
    }

    bool try_execute() override {
        if (queue_.empty()) {
            return false;
        }
        auto task = std::move(queue_.front());
        queue_.pop();
        task();
        return true;
    }

    [[nodiscard]] bool empty() const override { return queue_.empty(); }

    [[nodiscard]] std::size_t size() const { return queue_.size(); }

    std::size_t drain(std::size_t limit = std::numeric_limits<std::size_t>::max()) {
        std::size_t executed = 0;
        while (executed < limit && try_execute()) {
            ++executed;
        }
        return executed;
    }

  private:
    std::queue<std::function<void()>> queue_{};
};

class MockCommandScheduler final : public Interfaces::ICommandScheduler {
  public:
    QueueHandle create_queue(std::string_view name) override {
        const std::string key{name};
        if (auto it = queues_.find(key); it != queues_.end()) {
            return it->second;
        }
        auto queue = std::make_shared<MockCommandQueue>();
        queues_.emplace(key, queue);
        return queue;
    }

    QueueHandle get_queue(std::string_view name) override {
        const std::string key{name};
        if (auto it = queues_.find(key); it != queues_.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool remove_queue(std::string_view name) override {
        return queues_.erase(std::string{name}) > 0;
    }

    bool contains(std::string_view name) const override {
        return queues_.contains(std::string{name});
    }

    std::size_t pump(std::string_view name, std::size_t limit = std::numeric_limits<std::size_t>::max()) {
        if (auto it = queues_.find(std::string{name}); it != queues_.end()) {
            return it->second->drain(limit);
        }
        return 0;
    }

    std::size_t pump_all(std::size_t limit_per_queue = std::numeric_limits<std::size_t>::max()) {
        std::size_t total = 0;
        for (auto& [_, queue] : queues_) {
            total += queue->drain(limit_per_queue);
        }
        return total;
    }

    void clear() { queues_.clear(); }

  private:
    std::unordered_map<std::string, std::shared_ptr<MockCommandQueue>> queues_{};
};

} // namespace Corona::Testing
