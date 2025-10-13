#include <AnimationSystem.h>
#include <AudioSystem.h>
#include <CoronaEngineAPI.h>
#include <DisplaySystem.h>
#include <Engine.h>
#include <RenderingSystem.h>
#include <corona_logger.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <entt/entt.hpp>
#include <thread>
#include <vector>

namespace {
using clock_type = std::chrono::steady_clock;
constexpr double kTargetFps = 120.0;
constexpr std::chrono::duration<double> kFrameDuration{1.0 / kTargetFps};

std::atomic<bool> g_running{true};

void handle_signal(int) noexcept {
    g_running.store(false, std::memory_order_relaxed);
}
}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    std::signal(SIGINT, handle_signal);

    entt::registry registry;

    std::vector<entt::entity> base_entities;
    base_entities.reserve(100);
    for (size_t i = 0; i < 100; ++i) {
        auto entity = registry.create();
        base_entities.push_back(entity);
        registry.emplace<CoronaEngineAPI::RenderTag>(entity);
        registry.emplace<CoronaEngineAPI::AnimationTag>(entity);
        registry.emplace<CoronaEngineAPI::AudioTag>(entity);
        registry.emplace<CoronaEngineAPI::DisplayTag>(entity);
    }

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

    engine.register_system<Corona::AnimationSystem>();
    engine.register_system<Corona::RenderingSystem>();
    engine.register_system<Corona::AudioSystem>();
    engine.register_system<Corona::DisplaySystem>();

    auto& animation_system = engine.get_system<Corona::AnimationSystem>();
    auto& rendering_system = engine.get_system<Corona::RenderingSystem>();
    auto& audio_system = engine.get_system<Corona::AudioSystem>();
    auto& display_system = engine.get_system<Corona::DisplaySystem>();

    engine.start_systems();
    CE_LOG_INFO("All systems started");

    bool rendering_running = true;
    bool animation_running = true;
    bool audio_running = true;
    bool display_running = true;

    auto update_system = [](bool should_run, bool& is_running, auto& system) {
        if (should_run == is_running) {
            return;
        }
        if (should_run) {
            system.start();
        } else {
            system.stop();
        }
        is_running = should_run;
    };

    CE_LOG_INFO("Entering main loop (press Ctrl+C to exit)...");
    while (g_running.load(std::memory_order_relaxed)) {
        const auto frame_start = clock_type::now();

        // TODO: enqueue per-frame engine work (input, simulation, rendering, etc.).

        static int frame_counter = 0;
        ++frame_counter;

        auto toggle_tag_cycle = [&](auto tag_token,
                                    int cycle_frames,
                                    int add_offset,
                                    const char* label) {
            using Tag = std::decay_t<decltype(tag_token)>;
            const int phase = frame_counter % cycle_frames;

            if (phase == 0) {
                size_t removed = 0;
                for (auto entity : base_entities) {
                    if (registry.any_of<Tag>(entity)) {
                        registry.remove<Tag>(entity);
                        ++removed;
                    }
                }
                if (removed > 0) {
                    CE_LOG_DEBUG("Removed {} components from {} entities", label, static_cast<uint32_t>(removed));
                }
            } else if (phase == add_offset) {
                size_t added = 0;
                for (auto entity : base_entities) {
                    if (!registry.any_of<Tag>(entity)) {
                        registry.emplace<Tag>(entity);
                        ++added;
                    }
                }
                if (added > 0) {
                    CE_LOG_DEBUG("Added {} components to {} entities", label, static_cast<uint32_t>(added));
                }
            }
        };

        toggle_tag_cycle(CoronaEngineAPI::RenderTag{}, 600, 300, "RenderTag");
        toggle_tag_cycle(CoronaEngineAPI::AnimationTag{}, 720, 360, "AnimationTag");
        toggle_tag_cycle(CoronaEngineAPI::AudioTag{}, 840, 420, "AudioTag");
        toggle_tag_cycle(CoronaEngineAPI::DisplayTag{}, 960, 480, "DisplayTag");

        const bool has_render_entities = !registry.storage<CoronaEngineAPI::RenderTag>().empty();
        const bool has_animation_entities = !registry.storage<CoronaEngineAPI::AnimationTag>().empty();
        const bool has_audio_entities = !registry.storage<CoronaEngineAPI::AudioTag>().empty();
        const bool has_display_entities = !registry.storage<CoronaEngineAPI::DisplayTag>().empty();

        update_system(has_render_entities, rendering_running, rendering_system);
        update_system(has_animation_entities, animation_running, animation_system);
        update_system(has_audio_entities, audio_running, audio_system);
        update_system(has_display_entities, display_running, display_system);

        const auto frame_elapsed = clock_type::now() - frame_start;
        if (frame_elapsed < kFrameDuration) {
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
