#include "LoggerService.h"

namespace Corona::Core {

namespace {
Corona::LogLevel to_log_level(Interfaces::ILogger::Level level) {
    using Level = Interfaces::ILogger::Level;
    switch (level) {
        case Level::Trace:
            return Corona::LogLevel::kTrace;
        case Level::Debug:
            return Corona::LogLevel::kDebug;
        case Level::Info:
            return Corona::LogLevel::kInfo;
        case Level::Warn:
            return Corona::LogLevel::kWarn;
        case Level::Error:
            return Corona::LogLevel::kError;
        case Level::Critical:
            return Corona::LogLevel::kCritical;
    }
    return Corona::LogLevel::kInfo;
}
}  // namespace

void LoggerService::log(Level level, std::string_view message) {
    Corona::Logger::log(to_log_level(level), message);
}

}  // namespace Corona::Core
