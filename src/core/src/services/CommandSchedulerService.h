#pragma once

#include <corona/interfaces/Services.h>

#include <corona/threading/SafeCommandQueue.h>

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Corona::Core {

class CommandSchedulerService final : public Interfaces::ICommandScheduler {
  public:
    CommandSchedulerService() = default;
    ~CommandSchedulerService() override = default;

    QueueHandle create_queue(std::string_view name) override;
    QueueHandle get_queue(std::string_view name) override;
    bool remove_queue(std::string_view name) override;
    bool contains(std::string_view name) const override;

    void adopt_queue(std::string_view name, std::shared_ptr<SafeCommandQueue> queue);
    SafeCommandQueue& require_queue(std::string_view name);
    void clear();

  private:
    struct QueueRecord {
        std::shared_ptr<SafeCommandQueue> queue;
        std::weak_ptr<Interfaces::ICommandQueue> adapter;
    };

    std::shared_ptr<Interfaces::ICommandQueue> ensure_adapter(QueueRecord& record);

    QueueRecord& ensure_record(std::string_view name);
    QueueRecord* find_record(std::string_view name);
    const QueueRecord* find_record(std::string_view name) const;

    std::shared_ptr<SafeCommandQueue> ensure_queue_ptr(QueueRecord& record);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, QueueRecord> queues_;
};

} // namespace Corona::Core
