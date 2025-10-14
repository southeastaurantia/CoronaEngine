#include <Engine.h>
#include <corona_logger.h>

#include <atomic>
#include <csignal>

#include "RuntimeLoop.h"

namespace {
std::atomic<bool> g_running{true};

void handle_signal(int) noexcept {
    g_running.store(false, std::memory_order_relaxed);
}
}  // namespace

int main(int argc, char** argv) {
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

    auto& engine = Corona::Engine::instance();
    engine.init(log_config);
    CE_LOG_INFO("Engine initialized");

    RuntimeLoop runtime_loop{engine};
    runtime_loop.initialize();
    CE_LOG_INFO("All systems started");

    runtime_loop.run(g_running);

    runtime_loop.shutdown();

    engine.shutdown();
    CE_LOG_INFO("Engine shutdown complete");

    Corona::Logger::flush();
    return 0;
}
