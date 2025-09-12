// CoronaEngine - Core logging facade
// Header keeps backend details private to allow future swap of logging library.

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
// C++20 source location for capturing file/line/function at callsite
#include <source_location>

// Use spdlog's bundled fmt for safe, header-only formatting without adding a new dependency.
// This avoids exposing spdlog itself while enabling type-safe formatting APIs.
#include <spdlog/fmt/bundled/core.h>
#include <spdlog/fmt/bundled/format.h>

namespace Corona
{
    enum class LogLevel : uint8_t
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5,
        Off = 6
    };

    struct LogConfig
    {
        bool enableConsole = true;
        bool enableFile = false;
        // Rotating file sink settings (used when enableFile = true)
        std::string filePath = "logs/Corona.log";
        std::size_t maxFileSizeBytes = 5 * 1024 * 1024; // 5MB
        std::size_t maxFiles = 3;

        // Async logging (if supported by backend)
        bool async = false;

        // Pattern string, see backend docs (e.g., spdlog pattern syntax)
        // Header: [time][LEVEL][file:line] message
        // LEVEL is uppercased and fixed 5 chars (via custom level names below), also padded in pattern.
        std::string pattern = "%^[%Y-%m-%d %H:%M:%S.%e][%n][%-5!l][%g:%#] %v%$";

        // Initial level
        LogLevel level = LogLevel::Debug;
    };

    // Backend interface (hidden from users; forward-declared only in header)
    class ILogBackend;

    class Logger
    {
      public:
        // Initialize global logger with configuration (idempotent)
        static void Init(const LogConfig &config = {});
        static void Shutdown();

        static void SetLevel(LogLevel level);
        static LogLevel GetLevel();

        // Formatted logging helpers (fmt-style). Avoid exposing backend types.
        template <typename... Args>
        static void Trace(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Trace, fmtStr, std::forward<Args>(args)...);
        }

        // Location-aware overloads (location first to allow defaulted macro injection)
        template <typename... Args>
        static void Trace(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Trace, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Debug(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Debug, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Debug(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Debug, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Info(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Info, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Info(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Info, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Warn, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Warn, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Error, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Error, loc, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Critical(fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Critical, fmtStr, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Critical(const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            LogFormatted(LogLevel::Critical, loc, fmtStr, std::forward<Args>(args)...);
        }

        static void Flush();

        // Raw string logging (already formatted)
        static void Log(LogLevel level, std::string_view message);
        static void Log(LogLevel level, std::string_view message, const std::source_location &loc);

      private:
        static std::shared_ptr<ILogBackend> GetOrCreateBackend();

        template <typename... Args>
        static inline void LogFormatted(LogLevel level, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            Log(level, fmt::vformat(fmtStr, fmt::make_format_args(args...)));
        }

        template <typename... Args>
        static inline void LogFormatted(LogLevel level, const std::source_location &loc, fmt::format_string<Args...> fmtStr, Args &&...args)
        {
            Log(level, fmt::vformat(fmtStr, fmt::make_format_args(args...)), loc);
        }
    };
} // namespace Corona

// Compile-time filtering
// New: independent per-level switches CE_LOG_LEVEL_TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL (1=enabled, 0=disabled)
// Backward compatible: if not provided, derive from legacy LOG_LEVEL threshold
// Legacy mapping: 0-Debug(Trace,Debug enabled), 1-Info, 2-Warn, 3-Error (Error+Critical), 4-Off
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
#define CE_LOG_TRACE(...) ::Corona::Logger::Trace(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_TRACE(...) (void)0
#endif

#if CE_LOG_LEVEL_DEBUG
#define CE_LOG_DEBUG(...) ::Corona::Logger::Debug(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_DEBUG(...) (void)0
#endif

#if CE_LOG_LEVEL_INFO
#define CE_LOG_INFO(...) ::Corona::Logger::Info(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_INFO(...) (void)0
#endif

#if CE_LOG_LEVEL_WARN
#define CE_LOG_WARN(...) ::Corona::Logger::Warn(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_WARN(...) (void)0
#endif

#if CE_LOG_LEVEL_ERROR
#define CE_LOG_ERROR(...) ::Corona::Logger::Error(std::source_location::current(), __VA_ARGS__)
#else
#define CE_LOG_ERROR(...) (void)0
#endif

#if CE_LOG_LEVEL_CRITICAL
#define CE_LOG_CRITICAL(...)                                                  \
    ::Corona::Logger::Critical(std::source_location::current(), __VA_ARGS__); \
    throw std::runtime_error("Critical error occurred!")
#else
#define CE_LOG_CRITICAL(...) (void)0
#endif