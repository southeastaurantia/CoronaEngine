#pragma once

#include <string_view>

namespace corona::framework::services::logging {

enum class log_level {
    trace = 0,
    debug,
    info,
    warn,
    error,
    critical
};

[[nodiscard]] std::string_view to_string(log_level level) noexcept;

}  // namespace corona::framework::services::logging
