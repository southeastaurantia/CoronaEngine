#pragma once

#include <memory>
#include <string_view>

#include "corona/framework/services/logging/log_level.h"

namespace corona::framework::services::logging {

class logger {
   public:
    virtual ~logger() = default;

    virtual void log(log_level level, std::string_view message) = 0;

    void trace(std::string_view message) { log(log_level::trace, message); }
    void debug(std::string_view message) { log(log_level::debug, message); }
    void info(std::string_view message) { log(log_level::info, message); }
    void warn(std::string_view message) { log(log_level::warn, message); }
    void error(std::string_view message) { log(log_level::error, message); }
    void critical(std::string_view message) { log(log_level::critical, message); }
};

using logger_ptr = std::shared_ptr<logger>;

}  // namespace corona::framework::services::logging
