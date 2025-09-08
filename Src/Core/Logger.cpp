// Logger.cpp
#include "Logger.h"

// 所有的spdlog头文件只在这里被包含
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace Corona
{
    // 将spdlog的实现封装在一个私有类中
    class SpdlogImpl final : public ILogger
    {
      public:
        SpdlogImpl()
        {
            // 这部分代码和之前基本一样
            try
            {
                std::vector<spdlog::sink_ptr> sinks;
                const auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
                sinks.push_back(console_sink);

                const auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/corona-engine.log", 1024 * 1024 * 5, 10);
                file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
                sinks.push_back(file_sink);

                logger = std::make_shared<spdlog::logger>("CoronaEngine", begin(sinks), end(sinks));
                logger->set_level(spdlog::level::trace);
                logger->flush_on(spdlog::level::info);
                spdlog::register_logger(logger);
            }
            catch (const spdlog::spdlog_ex &ex)
            {
                fprintf(stderr, "Log initialization failed: %s\n", ex.what());
            }
        }

        ~SpdlogImpl() override
        {
            spdlog::shutdown();
        }

        // 实现接口的log方法
        void log(const LogLevel level, const std::string &message) override
        {
            // location信息在这里丢失了，所以最好用下面的重载
            logger->log(toSpdlogLevel(level), message);
        }

        // 实现带有source_location的log方法
        void log(const std::source_location &location, const LogLevel level, const std::string &message) override
        {
            // spdlog 2.0才原生支持 source_loc，这里我们手动格式化
            const std::string formatted_message = fmt::format("[{}:{}] {}",
                                                        location.file_name(),
                                                        location.line(),
                                                        message);
            logger->log(toSpdlogLevel(level), formatted_message);
        }

      private:
        // 辅助函数，将我们的LogLevel转换为spdlog的level
        static spdlog::level::level_enum toSpdlogLevel(LogLevel level)
        {
            switch (level)
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
            }
            return spdlog::level::off;
        }

        std::shared_ptr<spdlog::logger> logger;
    };

    // --- Logger类的实现 ---
    Logger &Logger::inst()
    {
        static Logger instance;
        return instance;
    }

    Logger::Logger()
    {
        // 这里是关键：我们创建了SpdlogImpl的实例
        // 如果要更换日志库，只需要改变这一行！
        pimpl = std::make_unique<SpdlogImpl>();
    }

    Logger::~Logger()
    {
        // unique_ptr会自动管理内存
    }
} // namespace Corona