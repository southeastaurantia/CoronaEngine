#include "corona/framework/services/logging/logging_setup.h"

#include <fast_io.h>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "corona/framework/services/logging/console_sink.h"
#include "corona/framework/services/logging/file_sink.h"
#include "corona/framework/services/logging/log_sink.h"

namespace corona::framework::services::logging {

namespace {

std::string format_record(const log_record& record) {
    const auto timestamp = record.timestamp;
    const auto as_time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &as_time_t);
#else
    localtime_r(&as_time_t, &tm);
#endif

    char time_buffer[32]{};
    if (std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S", &tm) == 0) {
        time_buffer[0] = '\0';
    }

    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    const auto millis_value = static_cast<int>(millis.count());
    char millis_buffer[4];
    millis_buffer[0] = static_cast<char>('0' + (millis_value / 100) % 10);
    millis_buffer[1] = static_cast<char>('0' + (millis_value / 10) % 10);
    millis_buffer[2] = static_cast<char>('0' + millis_value % 10);
    millis_buffer[3] = '\0';

    char thread_buffer[32];
    auto thread_numeric = static_cast<std::size_t>(std::hash<std::thread::id>{}(record.thread_id));
    auto [thread_out, thread_ec] = std::to_chars(thread_buffer, thread_buffer + (sizeof(thread_buffer) - 1), thread_numeric);
    if (thread_ec != std::errc{}) {
        thread_buffer[0] = '?';
        thread_out = thread_buffer + 1;
    }
    *thread_out = '\0';

    std::string_view thread_view{thread_buffer, static_cast<std::size_t>(thread_out - thread_buffer)};

    std::string_view time_view{time_buffer, std::strlen(time_buffer)};
    std::string_view millis_view{millis_buffer, 3};

    return fast_io::concat(time_view,
                           ".", millis_view,
                           " [", to_string(record.level),
                           "] (tid ", thread_view,
                           ") ", record.message);
}

class sink_logger final : public logger {
   public:
    sink_logger(std::vector<std::shared_ptr<log_sink>> sinks,
                std::size_t queue_capacity,
                std::chrono::milliseconds flush_period)
        : sinks_(std::move(sinks)),
          queue_capacity_(std::max<std::size_t>(queue_capacity, static_cast<std::size_t>(1))),
          flush_period_(flush_period),
          running_(true) {
        worker_ = std::thread([this]() { worker_loop(); });
    }

    ~sink_logger() override {
        stop();
    }

    void log(log_level level, std::string_view message) override {
        if (!running_.load(std::memory_order_acquire)) {
            return;
        }

        log_record record;
        record.level = level;
        record.message.assign(message.data(), message.size());
        record.timestamp = std::chrono::system_clock::now();
        record.thread_id = std::this_thread::get_id();

        std::string formatted = format_record(record);

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (queue_.size() >= queue_capacity_) {
                const auto dropped = dropped_messages_.fetch_add(1, std::memory_order_relaxed);
                if (dropped == 0) {
                    fast_io::io::print(fast_io::err(),
                                       "sink_logger queue overflow; dropping log messages (capacity ",
                                       queue_capacity_,
                                       ")\n");
                }
                return;
            }
            queue_.push_back({std::move(record), std::move(formatted)});
        }

        queue_cv_.notify_one();
    }

   private:
    struct queued_item {
        log_record record;
        std::string formatted;
    };

    void worker_loop() {
        auto next_flush = std::chrono::steady_clock::time_point::max();
        if (flush_period_.count() > 0) {
            next_flush = std::chrono::steady_clock::now() + flush_period_;
        }

        std::unique_lock<std::mutex> lock(queue_mutex_);
        while (running_.load(std::memory_order_acquire) || !queue_.empty()) {
            if (queue_.empty()) {
                if (flush_period_.count() > 0) {
                    queue_cv_.wait_until(lock, next_flush, [this]() {
                        return !running_.load(std::memory_order_acquire) || !queue_.empty();
                    });
                    const auto now = std::chrono::steady_clock::now();
                    if (flush_period_.count() > 0 && now >= next_flush) {
                        lock.unlock();
                        flush_sinks();
                        lock.lock();
                        next_flush = std::chrono::steady_clock::now() + flush_period_;
                    }
                } else {
                    queue_cv_.wait(lock, [this]() {
                        return !running_.load(std::memory_order_acquire) || !queue_.empty();
                    });
                }

                if (!running_.load(std::memory_order_acquire) && queue_.empty()) {
                    break;
                }
            }

            while (!queue_.empty()) {
                auto item = std::move(queue_.front());
                queue_.pop_front();

                lock.unlock();
                for (auto& sink : sinks_) {
                    if (sink) {
                        sink->log(item.record, item.formatted);
                    }
                }
                if (flush_period_.count() > 0) {
                    next_flush = std::chrono::steady_clock::now() + flush_period_;
                }
                lock.lock();
            }
        }

        lock.unlock();
        flush_sinks();
    }

    void flush_sinks() {
        for (auto& sink : sinks_) {
            if (sink) {
                sink->flush();
            }
        }
    }

    void stop() {
        const bool was_running = running_.exchange(false, std::memory_order_acq_rel);
        if (!was_running) {
            return;
        }

        queue_cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
        flush_sinks();
    }

    std::vector<std::shared_ptr<log_sink>> sinks_;
    std::size_t queue_capacity_;
    std::chrono::milliseconds flush_period_;

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::deque<queued_item> queue_;

    std::atomic<bool> running_;
    std::atomic<std::size_t> dropped_messages_{0};
    std::thread worker_;
};

class null_logger final : public logger {
   public:
    void log(log_level, std::string_view) override {}
};

}  // namespace

logger_ptr register_logging_services(service::service_collection& collection, const logging_config& config) {
    std::vector<std::shared_ptr<log_sink>> sinks;
    sinks.reserve(2);

    if (config.enable_console) {
        auto sink = std::make_shared<console_sink>(config.console_level);
        sinks.push_back(std::move(sink));
    }

    if (config.enable_file && !config.file_path.empty()) {
        auto sink = std::make_shared<file_sink>(config.file_path, config.file_level, config.file_append, config.file_flush_on_log);
        sinks.push_back(std::move(sink));
    }

    logger_ptr result;
    if (sinks.empty()) {
        result = std::make_shared<null_logger>();
    } else {
        result = std::make_shared<sink_logger>(std::move(sinks), config.queue_capacity, config.flush_period);
    }

    collection.add_singleton<logger>(result);
    return result;
}

}  // namespace corona::framework::services::logging
