#include "CommandSchedulerService.h"

#include <stdexcept>
#include <utility>

namespace Corona::Core {

namespace {
class SafeCommandQueueAdapter final : public Interfaces::ICommandQueue {
  public:
    explicit SafeCommandQueueAdapter(std::shared_ptr<SafeCommandQueue> queue)
        : queue_(std::move(queue)) {}

    void enqueue(std::function<void()> command) override {
        if (!queue_) {
            return;
        }
        queue_->enqueue(std::move(command));
    }

    bool try_execute() override {
        return queue_ ? queue_->try_execute() : false;
    }

    bool empty() const override {
        return queue_ ? queue_->empty() : true;
    }

  private:
    std::shared_ptr<SafeCommandQueue> queue_;
};
} // namespace

Interfaces::ICommandScheduler::QueueHandle CommandSchedulerService::create_queue(std::string_view name) {
    std::lock_guard lock(mutex_);
    auto& record = ensure_record(name);
    ensure_queue_ptr(record);
    return ensure_adapter(record);
}

Interfaces::ICommandScheduler::QueueHandle CommandSchedulerService::get_queue(std::string_view name) {
    std::lock_guard lock(mutex_);
    if (auto* record = find_record(name)) {
        return ensure_adapter(*record);
    }
    return nullptr;
}

bool CommandSchedulerService::remove_queue(std::string_view name) {
    std::lock_guard lock(mutex_);
    return queues_.erase(std::string{name}) > 0;
}

bool CommandSchedulerService::contains(std::string_view name) const {
    std::lock_guard lock(mutex_);
    return queues_.contains(std::string{name});
}

void CommandSchedulerService::adopt_queue(std::string_view name, std::shared_ptr<SafeCommandQueue> queue) {
    if (!queue) {
        return;
    }
    std::lock_guard lock(mutex_);
    auto& record = ensure_record(name);
    record.queue = std::move(queue);
    record.adapter.reset();
}

SafeCommandQueue& CommandSchedulerService::require_queue(std::string_view name) {
    std::lock_guard lock(mutex_);
    auto* record = find_record(name);
    if (!record || !record->queue) {
        throw std::runtime_error("Command queue not found: " + std::string{name});
    }
    return *record->queue;
}

void CommandSchedulerService::clear() {
    std::lock_guard lock(mutex_);
    queues_.clear();
}

CommandSchedulerService::QueueRecord& CommandSchedulerService::ensure_record(std::string_view name) {
    auto key = std::string{name};
    return queues_[key];
}

CommandSchedulerService::QueueRecord* CommandSchedulerService::find_record(std::string_view name) {
    auto key = std::string{name};
    if (auto it = queues_.find(key); it != queues_.end()) {
        return &it->second;
    }
    return nullptr;
}

const CommandSchedulerService::QueueRecord* CommandSchedulerService::find_record(std::string_view name) const {
    auto key = std::string{name};
    if (auto it = queues_.find(key); it != queues_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::shared_ptr<Interfaces::ICommandQueue> CommandSchedulerService::ensure_adapter(QueueRecord& record) {
    if (auto existing = record.adapter.lock()) {
        return existing;
    }
    auto adapter = std::make_shared<SafeCommandQueueAdapter>(ensure_queue_ptr(record));
    record.adapter = adapter;
    return adapter;
}

std::shared_ptr<SafeCommandQueue> CommandSchedulerService::ensure_queue_ptr(QueueRecord& record) {
    if (!record.queue) {
        record.queue = std::make_shared<SafeCommandQueue>();
    }
    return record.queue;
}

} // namespace Corona::Core
