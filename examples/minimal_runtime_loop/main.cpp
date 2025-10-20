#include <corona/api/CoronaEngineAPI.h>
#include <corona/core/Engine.h>
#include <corona/systems/AnimationSystem.h>
#include <corona/systems/AudioSystem.h>
#include <corona/systems/DisplaySystem.h>
#include <corona/systems/RenderingSystem.h>
#include <corona_logger.h>
#include <engine/RuntimeLoop.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <vector>

#include "CustomLoop.h"
#include "DiagnosticsSystem.h"

namespace {
std::atomic<bool> g_running{true};
void handle_signal(int) noexcept { g_running.store(false, std::memory_order_relaxed); }

void request_systems(const std::vector<std::string>& systems) {
    (void)std::getenv("CORONA_RUNTIME_SYSTEMS");
    const std::string joined = Example::JoinNames(systems);
#if defined(_WIN32)
    _putenv_s("CORONA_RUNTIME_SYSTEMS", joined.c_str());
#else
    if (joined.empty()) {
        unsetenv("CORONA_RUNTIME_SYSTEMS");
    } else {
        setenv("CORONA_RUNTIME_SYSTEMS", joined.c_str(), 1);
    }
#endif
}

void bootstrap_diagnostics(Corona::Engine& engine, const std::vector<std::string>& requested_systems) {
    auto config = std::make_shared<Example::DiagnosticsConfig>();
    config->requested_systems = requested_systems;

    auto service = std::make_shared<Example::DiagnosticsService>(engine);
    service->set_requested(requested_systems);

    auto& locator = engine.kernel().services();
    locator.register_service(config);
    locator.register_service(service);

    Example::RegisterDiagnosticsPlugin(engine.system_registry());
}
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

    const std::vector<std::string> requested_systems{
        "AnimationSystem",
        Example::DiagnosticsSystem::kName,
        "AudioSystem",
        "RenderingSystem",
        "DisplaySystem"};
    request_systems(requested_systems);
    bootstrap_diagnostics(engine, requested_systems);

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
