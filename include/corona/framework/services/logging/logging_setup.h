#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>

#include "corona/framework/service/service_collection.h"
#include "log_level.h"
#include "logger.h"

namespace corona::framework::services::logging {

struct logging_config {
    bool enable_console = true;
    log_level console_level = log_level::info;

    bool enable_file = false;
    std::filesystem::path file_path;
    log_level file_level = log_level::info;
    bool file_append = true;
    bool file_flush_on_log = false;

    std::size_t queue_capacity = 8192;
    std::chrono::milliseconds flush_period = std::chrono::milliseconds{0};
};

logger_ptr register_logging_services(service::service_collection& collection, const logging_config& config);

}  // namespace corona::framework::services::logging
