#include "Engine.h"
#include <corona_logger.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

namespace
{
    using clock_type = std::chrono::steady_clock;
    constexpr double kTargetFps = 120.0;
    constexpr std::chrono::duration<double> kFrameDuration{1.0 / kTargetFps};

    std::atomic<bool> g_running{true};

    void handle_signal(int) noexcept
    {
        g_running.store(false, std::memory_order_relaxed);
    }
} // namespace

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    std::signal(SIGINT, handle_signal);

    // 初始化日志系统
    Corona::LogConfig log_config{};
    log_config.enable_console_ = true;
    log_config.enable_file_ = true;
    log_config.level_ = Corona::LogLevel::kDebug;
    Corona::Logger::init(log_config);
    
    CE_LOG_INFO("CoronaEngine starting...");
    
    auto &engine = Corona::Engine::instance();
    engine.init(log_config);
    CE_LOG_INFO("Engine initialized");
    
    engine.start_systems();
    CE_LOG_INFO("All systems started");

    CE_LOG_INFO("Entering main loop (press Ctrl+C to exit)...");
    while (g_running.load(std::memory_order_relaxed))
    {
        const auto frame_start = clock_type::now();

        // TODO: enqueue per-frame engine work (input, simulation, rendering, etc.).

        const auto frame_elapsed = clock_type::now() - frame_start;
        if (frame_elapsed < kFrameDuration)
        {
            std::this_thread::sleep_for(kFrameDuration - frame_elapsed);
        }
    }
    CE_LOG_INFO("Main loop exited");

    engine.stop_systems();
    CE_LOG_INFO("All systems stopped");
    
    engine.shutdown();
    CE_LOG_INFO("Engine shutdown complete");
    
    Corona::Logger::flush();
    return 0;
}
