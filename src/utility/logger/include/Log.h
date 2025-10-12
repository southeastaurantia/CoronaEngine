// CoronaEngine - 日志API（Facade）
// 仅暴露统一接口，隐藏具体后端，便于后续替换日志库。

#pragma once

#include <cstdint>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>

// 使用 spdlog 自带的 fmt（头文件实现），提供类型安全的格式化能力，同时不在接口层暴露 spdlog。
#include <spdlog/fmt/bundled/core.h>
#include <spdlog/fmt/bundled/format.h>

namespace Corona
{
    enum class LogLevel : uint8_t
    {
        kTrace = 0,
        kDebug = 1,
        kInfo = 2,
        kWarn = 3,
        kError = 4,
        kCritical = 5,
        kOff = 6
    };

    struct LogConfig
    {
        bool enable_console_ = true;
        bool enable_file_ = false;
    // 滚动文件输出配置（当 enable_file_ = true 生效）
        std::string file_path_ = "logs/Corona.log";
        std::size_t max_file_size_bytes_ = 5 * 1024 * 1024; // 5MB
        std::size_t max_files_ = 3;

        // 异步日志（若后端支持）
        bool async_ = false;

        // 日志格式字符串（参见后端语法，如 spdlog）：
        // 形如：[时间][Logger名][级别][文件:行] 消息
        std::string pattern_ = "%^[%Y-%m-%d %H:%M:%S.%e][%n][%-5!l][%g:%#] %v%$";

        // 初始日志级别
        LogLevel level_ = LogLevel::kDebug;
    };

    // Backend interface (hidden from users; forward-declared only in header)
    class ILogBackend;

    class Logger
    {
      public:
        // 以配置初始化全局日志器（幂等）
        static void init(const LogConfig &config = {});
        static void shutdown();

        static void set_level(LogLevel level);
        static LogLevel get_level();

        // 格式化日志（fmt 风格），不暴露后端类型
        template <typename... Args>
        static void trace(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kTrace, fmtStr, std::forward<Args>(args)...);
        }

        // 带位置信息重载（便于日志宏自动注入 source_location）
        template <typename... Args>
        static void trace(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kTrace, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void debug(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kDebug, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void debug(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kDebug, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void info(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kInfo, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void info(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kInfo, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void warn(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kWarn, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void warn(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kWarn, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void error(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kError, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void error(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kError, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void critical(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kCritical, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void critical(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log_formatted(LogLevel::kCritical, loc, fmtStr, std::forward<Args>(args)...);
        }

        static void flush();

        // 原始字符串日志（已格式化）
        static void log(LogLevel level, std::string_view message);
        static void log(LogLevel level, std::string_view message, const std::source_location &loc);

      private:
        static std::shared_ptr<ILogBackend> get_or_create_backend();

        template <typename... Args>
        static inline void log_formatted(LogLevel level, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log(level, fmt::vformat(fmtStr, fmt::make_format_args(args...)));
        }

        template <typename... Args>
        static inline void log_formatted(LogLevel level, const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            log(level, fmt::vformat(fmtStr, fmt::make_format_args(args...)), loc);
        }
    };
} // namespace Corona

// 编译期日志开关：
// - 独立的级别开关 CE_LOG_LEVEL_TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL（1 开启 / 0 关闭）
#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

#ifndef CE_LOG_LEVEL_TRACE
#if LOG_LEVEL <= 0
#define CE_LOG_LEVEL_TRACE 1
#else
#define CE_LOG_LEVEL_TRACE 0
#endif
#endif

#ifndef CE_LOG_LEVEL_DEBUG
#if LOG_LEVEL <= 1
#define CE_LOG_LEVEL_DEBUG 1
#else
#define CE_LOG_LEVEL_DEBUG 0
#endif
#endif

#ifndef CE_LOG_LEVEL_INFO
#if LOG_LEVEL <= 2
#define CE_LOG_LEVEL_INFO 1
#else
#define CE_LOG_LEVEL_INFO 0
#endif
#endif

#ifndef CE_LOG_LEVEL_WARN
#if LOG_LEVEL <= 3
#define CE_LOG_LEVEL_WARN 1
#else
#define CE_LOG_LEVEL_WARN 0
#endif
#endif

#ifndef CE_LOG_LEVEL_ERROR
#if LOG_LEVEL <= 4
#define CE_LOG_LEVEL_ERROR 1
#else
#define CE_LOG_LEVEL_ERROR 0
#endif
#endif

#ifndef CE_LOG_LEVEL_CRITICAL
#if LOG_LEVEL <= 5
#define CE_LOG_LEVEL_CRITICAL 1
#else
#define CE_LOG_LEVEL_CRITICAL 0
#endif
#endif

// Convenience macros (compiled out when disabled)
#if CE_LOG_LEVEL_TRACE
#define CE_LOG_TRACE(...) ::Corona::Logger::trace(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_TRACE(...) (void)0
#endif

#if CE_LOG_LEVEL_DEBUG
#define CE_LOG_DEBUG(...) ::Corona::Logger::debug(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_DEBUG(...) (void)0
#endif

#if CE_LOG_LEVEL_INFO
#define CE_LOG_INFO(...) ::Corona::Logger::info(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_INFO(...) (void)0
#endif

#if CE_LOG_LEVEL_WARN
#define CE_LOG_WARN(...) ::Corona::Logger::warn(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_WARN(...) (void)0
#endif

#if CE_LOG_LEVEL_ERROR
#define CE_LOG_ERROR(...) ::Corona::Logger::error(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_ERROR(...) (void)0
#endif

#if CE_LOG_LEVEL_CRITICAL
#define CE_LOG_CRITICAL(...) ::Corona::Logger::critical(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_CRITICAL(...) (void)0
#endif