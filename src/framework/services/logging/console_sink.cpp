#include "corona/framework/services/logging/console_sink.h"

#include <iostream>

namespace corona::framework::services::logging {

console_sink::console_sink(log_level min_level)
    : min_level_(min_level) {}

void console_sink::log(const log_record& record, std::string_view formatted) {
    if (record.level < min_level_.load(std::memory_order_relaxed)) {
        return;
    }

    std::lock_guard<std::mutex> guard(stream_mutex_);
    std::clog.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
    if (formatted.empty() || formatted.back() != '\n') {
        std::clog.put('\n');
    }
    std::clog.flush();
}

void console_sink::flush() {
    std::lock_guard<std::mutex> guard(stream_mutex_);
    std::clog.flush();
}

void console_sink::set_min_level(log_level level) noexcept {
    min_level_.store(level, std::memory_order_relaxed);
}

log_level console_sink::min_level() const noexcept {
    return min_level_.load(std::memory_order_relaxed);
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
