// CoronaEngine - Core logging implementation (spdlog backend)

#include "Log.h"

#include <atomic>
#include <mutex>
#include <vector>
// C++20 source_location
#include <source_location>

// Only this file includes spdlog; keeps it out of public headers.
#include <spdlog/async.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Singleton-like storage
static std::mutex g_mutex{};
static std::shared_ptr<Corona::ILogBackend> g_backend{nullptr};
static std::atomic<bool> g_inited{false};

namespace Corona
{
    // Backend interface hidden within translation unit
    class ILogBackend
    {
      public:
        virtual ~ILogBackend() = default;
        virtual void Configure(const LogConfig &cfg) = 0;
        virtual void SetLevel(LogLevel level) = 0;
        virtual LogLevel GetLevel() const = 0;
        virtual void Log(LogLevel level, std::string_view msg) = 0;
        virtual void Log(LogLevel level, std::string_view msg, const std::source_location &loc) = 0;
        virtual void Flush() = 0;
    };

    static spdlog::level::level_enum ToSpd(LogLevel lvl)
    {
        switch (lvl)
        {
        case LogLevel::Trace:
            return spdlog::level::trace;
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warn:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
        case LogLevel::Critical:
            return spdlog::level::critical;
        case LogLevel::Off:
            return spdlog::level::off;
        }
        return spdlog::level::info;
    }

    static LogLevel FromSpd(spdlog::level::level_enum lvl)
    {
        switch (lvl)
        {
        case spdlog::level::trace:
            return LogLevel::Trace;
        case spdlog::level::debug:
            return LogLevel::Debug;
        case spdlog::level::info:
            return LogLevel::Info;
        case spdlog::level::warn:
            return LogLevel::Warn;
        case spdlog::level::err:
            return LogLevel::Error;
        case spdlog::level::critical:
            return LogLevel::Critical;
        case spdlog::level::off:
            return LogLevel::Off;
        default:
            return LogLevel::Info;
        }
    }

    class SpdLogBackend final : public ILogBackend
    {
      public:
        SpdLogBackend()
        {
            // Ensure spdlog nothrow; avoid exceptions across engine
            spdlog::init_thread_pool(8192, 1); // small queue by default, will enable when async
        }

        ~SpdLogBackend() override
        {
            try
            {
                if (m_logger)
                    m_logger->flush();
            }
            catch (...)
            { /* swallow */
            }
        }

        void Configure(const LogConfig &cfg) override
        {
            std::vector<spdlog::sink_ptr> sinks;
            if (cfg.enableConsole)
            {
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sinks.emplace_back(console_sink);
            }
            if (cfg.enableFile)
            {
                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(cfg.filePath, cfg.maxFileSizeBytes, cfg.maxFiles);
                sinks.emplace_back(file_sink);
            }

            if (cfg.async)
            {
                m_logger = std::make_shared<spdlog::async_logger>("Corona", begin(sinks), end(sinks), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
                spdlog::register_logger(m_logger);
            }
            else
            {
                m_logger = std::make_shared<spdlog::logger>("Corona", begin(sinks), end(sinks));
                spdlog::register_logger(m_logger);
            }

            m_logger->set_pattern(cfg.pattern);
            m_logger->set_level(ToSpd(cfg.level));
            m_logger->flush_on(spdlog::level::warn);
        }

        void SetLevel(LogLevel level) override
        {
            if (m_logger)
                m_logger->set_level(ToSpd(level));
        }

        LogLevel GetLevel() const override
        {
            return m_logger ? FromSpd(m_logger->level()) : LogLevel::Info;
        }

        void Log(LogLevel level, std::string_view msg) override
        {
            if (!m_logger)
                return;
            switch (level)
            {
            case LogLevel::Trace:
                m_logger->trace("{}", msg);
                break;
            case LogLevel::Debug:
                m_logger->debug("{}", msg);
                break;
            case LogLevel::Info:
                m_logger->info("{}", msg);
                break;
            case LogLevel::Warn:
                m_logger->warn("{}", msg);
                break;
            case LogLevel::Error:
                m_logger->error("{}", msg);
                break;
            case LogLevel::Critical:
                m_logger->critical("{}", msg);
                break;
            case LogLevel::Off: /* no-op */
                break;
            }
        }

        void Log(LogLevel level, std::string_view msg, const std::source_location &loc) override
        {
            if (!m_logger)
                return;
            // Map to spdlog::source_loc to drive [%s:%#] and function formatting
            spdlog::source_loc sloc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()};
            switch (level)
            {
            case LogLevel::Trace:
                m_logger->log(sloc, spdlog::level::trace, "{}", msg);
                break;
            case LogLevel::Debug:
                m_logger->log(sloc, spdlog::level::debug, "{}", msg);
                break;
            case LogLevel::Info:
                m_logger->log(sloc, spdlog::level::info, "{}", msg);
                break;
            case LogLevel::Warn:
                m_logger->log(sloc, spdlog::level::warn, "{}", msg);
                break;
            case LogLevel::Error:
                m_logger->log(sloc, spdlog::level::err, "{}", msg);
                break;
            case LogLevel::Critical:
                m_logger->log(sloc, spdlog::level::critical, "{}", msg);
                break;
            case LogLevel::Off:
                break;
            }
        }

        void Flush() override
        {
            if (m_logger)
                m_logger->flush();
        }

      private:
        std::shared_ptr<spdlog::logger> m_logger;
    };

    static std::shared_ptr<ILogBackend> CreateBackend()
    {
        return std::make_shared<SpdLogBackend>();
    }

    void Logger::Init(const LogConfig &config)
    {
        if (g_inited.load(std::memory_order_acquire))
            return;
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_backend)
        {
            g_backend = CreateBackend();
            g_backend->Configure(config);
        }
        g_inited.store(true, std::memory_order_release);
    }

    void Logger::Shutdown()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_backend)
        {
            g_backend->Flush();
            g_backend.reset();
        }
        g_inited.store(false, std::memory_order_release);
        spdlog::shutdown();
    }

    void Logger::SetLevel(LogLevel level)
    {
        auto backend = GetOrCreateBackend();
        backend->SetLevel(level);
    }

    LogLevel Logger::GetLevel()
    {
        auto backend = GetOrCreateBackend();
        return backend->GetLevel();
    }

    void Logger::Log(LogLevel level, std::string_view message)
    {
        auto backend = GetOrCreateBackend();
        backend->Log(level, message);
    }

    void Logger::Log(LogLevel level, std::string_view message, const std::source_location &loc)
    {
        auto backend = GetOrCreateBackend();
        backend->Log(level, message, loc);
    }

    void Logger::Flush()
    {
        auto backend = GetOrCreateBackend();
        backend->Flush();
    }

    std::shared_ptr<ILogBackend> Logger::GetOrCreateBackend()
    {
        if (g_backend)
            return g_backend;
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_backend)
        {
            g_backend = CreateBackend();
            LogConfig defaultCfg{};
            g_backend->Configure(defaultCfg);
            g_inited.store(true, std::memory_order_release);
        }
        return g_backend;
    }
} // namespace Corona
