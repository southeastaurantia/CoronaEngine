#include "corona/framework/services/logging/file_sink.h"

#include <iostream>

namespace corona::framework::services::logging {

file_sink::file_sink(std::filesystem::path file_path,
                     log_level min_level,
                     bool append,
                     bool flush_on_write)
    : file_path_(std::move(file_path)),
      min_level_(min_level),
      append_(append),
      flush_on_write_(flush_on_write) {
    ready_ = ensure_stream();
}

file_sink::~file_sink() {
    std::lock_guard<std::mutex> guard(stream_mutex_);
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
}

void file_sink::log(const log_record& record, std::string_view formatted) {
    if (record.level < min_level_.load(std::memory_order_relaxed)) {
        return;
    }
    if (!ready_ && !ensure_stream()) {
        return;
    }

    std::lock_guard<std::mutex> guard(stream_mutex_);
    if (!stream_.is_open()) {
        return;
    }
    stream_.write(formatted.data(), static_cast<std::streamsize>(formatted.size()));
    if (formatted.empty() || formatted.back() != '\n') {
        stream_.put('\n');
    }
    if (flush_on_write_) {
        stream_.flush();
    }
}

void file_sink::flush() {
    std::lock_guard<std::mutex> guard(stream_mutex_);
    if (stream_.is_open()) {
        stream_.flush();
    }
}

void file_sink::set_min_level(log_level level) noexcept {
    min_level_.store(level, std::memory_order_relaxed);
}

log_level file_sink::min_level() const noexcept {
    return min_level_.load(std::memory_order_relaxed);
}

bool file_sink::ensure_stream() {
    std::lock_guard<std::mutex> guard(stream_mutex_);
    if (stream_.is_open()) {
        return true;
    }
    try {
        if (!file_path_.empty()) {
            auto parent = file_path_.parent_path();
            if (!parent.empty()) {
                std::filesystem::create_directories(parent);
            }
        }
        stream_.open(file_path_, append_ ? std::ios::app : std::ios::trunc);
        ready_ = stream_.is_open();
        if (!ready_) {
            std::cerr << "file_sink failed to open file: " << file_path_ << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "file_sink exception opening file '" << file_path_ << "': " << ex.what() << std::endl;
        ready_ = false;
    }
    return ready_;
}

}  // namespace corona::framework::services::logging
