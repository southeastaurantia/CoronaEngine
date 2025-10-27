#pragma once

#include <fast_io.h>
#include <fast_io_device.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>

#include "log_sink.h"

namespace corona::framework::services::logging {

class file_sink final : public log_sink {
   public:
    explicit file_sink(std::filesystem::path file_path,
                       log_level min_level = log_level::info,
                       bool append = true,
                       bool flush_on_write = true);
    ~file_sink() override;

    void log(const log_record& record, std::string_view formatted) override;
    void flush() override;

    void set_min_level(log_level level) noexcept;
    [[nodiscard]] log_level min_level() const noexcept;
    [[nodiscard]] const std::filesystem::path& file_path() const noexcept { return file_path_; }

   private:
    bool ensure_stream();

    std::filesystem::path file_path_;
    std::atomic<log_level> min_level_;
    bool append_;
    bool flush_on_write_;

    std::mutex stream_mutex_;
    std::unique_ptr<fast_io::obuf_file> stream_;
    bool ready_ = false;
};

}  // namespace corona::framework::services::logging
