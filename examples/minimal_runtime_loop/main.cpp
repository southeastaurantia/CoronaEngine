#include <CoronaEngineAPI.h>
#include <Engine.h>
#include <corona_logger.h>
#include <engine/RuntimeLoop.h>
#include <atomic>
#include <csignal>
#include <filesystem>
#include <RenderingSystem.h>
#include <AnimationSystem.h>
#include <AudioSystem.h>
#include <DisplaySystem.h>

#include "CustomLoop.h"

namespace {
std::atomic<bool> g_running{true};
void handle_signal(int) noexcept { g_running.store(false, std::memory_order_relaxed); }
}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    std::signal(SIGINT, handle_signal);

    // 初始化日志
    Corona::LogConfig log_cfg{};
    log_cfg.enable_console_ = true;
    log_cfg.enable_file_ = false;
    // 使用 Debug 级别，便于看到各系统的 start/stop 调试日志
    log_cfg.level_ = Corona::LogLevel::kDebug;

    Corona::Logger::init(log_cfg);

    CE_LOG_INFO("[example] CoronaEngine minimal_runtime_loop starting...");

    // 拿到引擎单例并初始化
    auto& engine = Corona::Engine::instance();
    engine.init(log_cfg);

    // 创建并运行自定义的 RuntimeLoop（在构造中设置了每帧回调）
    CE_LOG_INFO("[example] Initializing CustomLoop and starting systems...");
    CustomLoop loop{engine};
    loop.initialize();

    CE_LOG_INFO("[example] Systems initialized and running. Entering run loop.");

    auto& display_system = Corona::Engine::instance().get_system<Corona::DisplaySystem>();
    auto& animation_system = Corona::Engine::instance().get_system<Corona::AnimationSystem>();
    auto& audio_system = Corona::Engine::instance().get_system<Corona::AudioSystem>();
    auto& render_system = Corona::Engine::instance().get_system<Corona::RenderingSystem>();

    auto& render_queue = Corona::Engine::instance().get_queue(render_system.name());
    auto& animation_queue = Corona::Engine::instance().get_queue(animation_system.name());
    auto& audio_queue = Corona::Engine::instance().get_queue(audio_system.name());
    auto& display_queue = Corona::Engine::instance().get_queue(display_system.name());

    loop.run(g_running);
    loop.shutdown();

    engine.shutdown();
    Corona::Logger::flush();
    return 0;
}
