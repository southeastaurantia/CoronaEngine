#include "corona/framework/services/logging/file_sink.h"

#include <fast_io.h>
#include <fast_io_device.h>

#include <cstdio>
#include <exception>

namespace corona::framework::services::logging {

namespace {

constexpr fast_io::open_mode make_open_mode(bool append) noexcept {
    auto mode = fast_io::open_mode::out | fast_io::open_mode::creat;
    mode |= append ? fast_io::open_mode::app : fast_io::open_mode::trunc;
    return mode;
}

}  // namespace

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
    if (stream_) {
        try {
            fast_io::flush(*stream_);
        } catch (...) {
        }
        stream_.reset();
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
    if (!stream_) {
        return;
    }
    fast_io::io::print(*stream_, formatted);
    if (formatted.empty() || formatted.back() != '\n') {
        fast_io::io::print(*stream_, fast_io::mnp::chvw('\n'));
    }
    if (flush_on_write_) {
        try {
            fast_io::flush(*stream_);
        } catch (...) {
        }
    }
}

void file_sink::flush() {
    std::lock_guard<std::mutex> guard(stream_mutex_);
    if (stream_) {
        try {
            fast_io::flush(*stream_);
        } catch (...) {
        }
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
    if (stream_) {
        return true;
    }
    try {
        if (!file_path_.empty()) {
            auto parent = file_path_.parent_path();
            if (!parent.empty()) {
                std::filesystem::create_directories(parent);
            }
        }
        stream_ = std::make_unique<fast_io::obuf_file>(file_path_, make_open_mode(append_));
        ready_ = static_cast<bool>(stream_);
    } catch (const std::exception& ex) {
        ready_ = false;
        stream_.reset();
        std::fputs("file_sink exception opening log file: ", stderr);
        std::fputs(ex.what(), stderr);
        std::fputc('\n', stderr);
    }
    return ready_;
}

}  // namespace corona::framework::services::logging
