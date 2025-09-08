#pragma once

#include <source_location> // C++20 for source location
#include <string>

namespace Corona
{
    // 定义一个通用的日志级别
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

    class ILogger
    {
      public:
        virtual ~ILogger() = default;

        // 一个通用的日志记录方法
        virtual void log(LogLevel level, const std::string &message) = 0;
        virtual void log(const std::source_location &location, LogLevel level, const std::string &message) = 0;
    };
} // namespace Corona