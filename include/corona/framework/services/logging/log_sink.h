#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <thread>

#include "log_level.h"

namespace corona::framework::services::logging {

struct log_record {
    log_level level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id thread_id;
};

class log_sink {
   public:
    virtual ~log_sink() = default;
    virtual void log(const log_record& record, std::string_view formatted) = 0;
    virtual void flush() {}
};

}  // namespace corona::framework::services::logging
