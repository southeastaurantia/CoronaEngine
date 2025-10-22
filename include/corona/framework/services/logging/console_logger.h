#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "corona/framework/service/service_collection.h"
#include "corona/framework/services/logging/logger.h"

namespace corona::framework::services::logging {

class console_logger final : public logger {
   public:
    explicit console_logger(log_level min_level = log_level::info);
    ~console_logger() override;

    console_logger(const console_logger&) = delete;
    console_logger& operator=(const console_logger&) = delete;
    console_logger(console_logger&&) = delete;
    console_logger& operator=(console_logger&&) = delete;

    void log(log_level level, std::string_view message) override;

    void set_min_level(log_level level) noexcept;
    [[nodiscard]] log_level min_level() const noexcept;

   private:
    [[nodiscard]] static bool should_emit(log_level level, log_level threshold) noexcept;

    struct log_entry {
        log_level level;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
        std::thread::id thread_id;
    };

    void enqueue(log_entry entry);
    void worker_loop();
    void write_entry(const log_entry& entry);

    std::atomic<bool> running_;
    std::atomic<log_level> min_level_;
    std::size_t max_queue_size_;

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::deque<log_entry> queue_;
    std::thread worker_;
};

std::shared_ptr<logger> register_console_logger(service::service_collection& collection,
                                                log_level min_level = log_level::info);

}  // namespace corona::framework::services::logging
