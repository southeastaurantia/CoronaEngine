// Logger.h
#pragma once

#include "ILogger.h"
#include <memory>
#include <string>

// 格式化库，可以使用<fmt> (C++20) 或 spdlog附带的 {fmt}
#include "spdlog/fmt/fmt.h"

namespace Corona
{
    class Logger
    {
      public:
        static Logger &inst();

        // 提供一个模板化的方法来处理格式化字符串
        template <typename... Args>
        void log(const std::source_location &location, LogLevel level, fmt::format_string<Args...> fmt, Args &&...args)
        {
            if (pimpl)
            {
                const std::string message = fmt::format(fmt, std::forward<Args>(args)...);
                pimpl->log(location, level, message);
            }
        }

      private:
        Logger();
        ~Logger();

        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

        std::unique_ptr<ILogger> pimpl; // 指向实现（PIMPL）
    };

} // namespace Corona

#define LOG_TRACE(...) Corona::Logger::inst().log(std::source_location::current(), Corona::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) Corona::Logger::inst().log(std::source_location::current(), Corona::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) Corona::Logger::inst().log(std::source_location::current(), Corona::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) Corona::Logger::inst().log(std::source_location::current(), Corona::LogLevel::Warn, __VA_ARGS__)
#define LOG_ERROR(...) Corona::Logger::inst().log(std::source_location::current(), Corona::LogLevel::Error, __VA_ARGS__)
#define LOG_CRITICAL(...) Corona::Logger::inst().log(std::source_location::current(), Corona::LogLevel::Critical, __VA_ARGS__)