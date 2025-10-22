#pragma once

#include <atomic>
#include <mutex>

#include "log_sink.h"

namespace corona::framework::services::logging {

class console_sink final : public log_sink {
   public:
    explicit console_sink(log_level min_level = log_level::info);

    void log(const log_record& record, std::string_view formatted) override;
    void flush() override;

    void set_min_level(log_level level) noexcept;
    [[nodiscard]] log_level min_level() const noexcept;

   private:
    std::atomic<log_level> min_level_;
    std::mutex stream_mutex_;
};

std::string_view to_string(log_level level) noexcept;

}  // namespace corona::framework::services::logging
