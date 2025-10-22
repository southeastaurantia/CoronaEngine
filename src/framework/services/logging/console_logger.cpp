#include "corona/framework/services/logging/console_logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <utility>

#include "corona/framework/service/service_provider.h"

namespace corona::framework::services::logging {

namespace {

constexpr int level_value(log_level level) noexcept {
    return static_cast<int>(level);
}

constexpr std::size_t kDefaultQueueCapacity = 4096;

}  // namespace

console_logger::console_logger(log_level min_level)
    : running_(true),
      min_level_(min_level),
      max_queue_size_(kDefaultQueueCapacity),
      worker_([this] { worker_loop(); }) {}

console_logger::~console_logger() {
    running_.store(false, std::memory_order_release);
    queue_cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    // Drain any remaining entries synchronously.
    std::deque<log_entry> remaining;
    {
        std::lock_guard<std::mutex> guard(queue_mutex_);
        remaining.swap(queue_);
    }
    for (const auto& entry : remaining) {
        write_entry(entry);
    }
}

void console_logger::log(log_level level, std::string_view message) {
    auto threshold = min_level_.load(std::memory_order_relaxed);
    if (!should_emit(level, threshold)) {
        return;
    }

    log_entry entry{};
    entry.level = level;
    entry.timestamp = std::chrono::system_clock::now();
    entry.thread_id = std::this_thread::get_id();
    entry.message.assign(message.data(), message.size());

    enqueue(std::move(entry));
}

void console_logger::set_min_level(log_level level) noexcept {
    min_level_.store(level, std::memory_order_relaxed);
}

log_level console_logger::min_level() const noexcept {
    return min_level_.load(std::memory_order_relaxed);
}

bool console_logger::should_emit(log_level level, log_level threshold) noexcept {
    return level_value(level) >= level_value(threshold);
}

void console_logger::enqueue(log_entry entry) {
    {
        std::lock_guard<std::mutex> guard(queue_mutex_);
        if (queue_.size() >= max_queue_size_) {
            queue_.pop_front();
        }
        queue_.push_back(std::move(entry));
    }
    queue_cv_.notify_one();
}

void console_logger::worker_loop() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    while (true) {
        queue_cv_.wait(lock, [this] {
            return !queue_.empty() || !running_.load(std::memory_order_acquire);
        });

        if (queue_.empty()) {
            if (!running_.load(std::memory_order_acquire)) {
                break;
            }
            continue;
        }

        auto entry = std::move(queue_.front());
        queue_.pop_front();

        lock.unlock();
        write_entry(entry);
        lock.lock();
    }

    while (!queue_.empty()) {
        auto entry = std::move(queue_.front());
        queue_.pop_front();
        lock.unlock();
        write_entry(entry);
        lock.lock();
    }
}

void console_logger::write_entry(const log_entry& entry) {
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(entry.timestamp);
    auto time = std::chrono::system_clock::to_time_t(seconds);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp - seconds);

    std::tm tm_buffer{};

#if defined(_WIN32)
    localtime_s(&tm_buffer, &time);
#else
    localtime_r(&time, &tm_buffer);
#endif

    std::ostringstream stream;
    stream << std::put_time(&tm_buffer, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setw(3) << std::setfill('0') << static_cast<int>(ms.count()) << std::setfill(' ')
           << " [" << to_string(entry.level) << "]"
           << " [tid=" << entry.thread_id << "] "
           << entry.message;

    std::clog << stream.str() << std::endl;
}

std::shared_ptr<logger> register_console_logger(service::service_collection& collection, log_level min_level) {
    auto logger_instance = std::make_shared<console_logger>(min_level);
    collection.add_singleton<logger>([logger_instance](service::service_provider&) {
        return logger_instance;
    });
    return logger_instance;
}

std::string_view to_string(log_level level) noexcept {
    switch (level) {
        case log_level::trace:
            return "TRACE";
        case log_level::debug:
            return "DEBUG";
        case log_level::info:
            return "INFO";
        case log_level::warn:
            return "WARN";
        case log_level::error:
            return "ERROR";
        case log_level::critical:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

}  // namespace corona::framework::services::logging
