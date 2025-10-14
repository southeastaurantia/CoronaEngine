#include "RuntimeLoop.h"

#include <AnimationSystem.h>
#include <AudioSystem.h>
#include <CoronaEngineAPI.h>
#include <DisplaySystem.h>
#include <Engine.h>
#include <RenderingSystem.h>

#include <chrono>
#include <thread>

namespace {
using clock_type = std::chrono::steady_clock;
constexpr double kTargetFps = 120.0;
constexpr std::chrono::duration<double> kFrameDuration{1.0 / kTargetFps};
}  // namespace

RuntimeLoop::RuntimeLoop(Corona::Engine& engine) : engine_(engine) {
    base_entities_.reserve(100);
}

void RuntimeLoop::initialize() {
    build_base_entities();

    engine_.register_system<Corona::AnimationSystem>();
    engine_.register_system<Corona::RenderingSystem>();
    engine_.register_system<Corona::AudioSystem>();
    engine_.register_system<Corona::DisplaySystem>();

    animation_system_ = &engine_.get_system<Corona::AnimationSystem>();
    rendering_system_ = &engine_.get_system<Corona::RenderingSystem>();
    audio_system_ = &engine_.get_system<Corona::AudioSystem>();
    display_system_ = &engine_.get_system<Corona::DisplaySystem>();

    on_initialize();

    engine_.start_systems();

    animation_running_ = true;
    rendering_running_ = true;
    audio_running_ = true;
    display_running_ = true;
}

void RuntimeLoop::run(std::atomic<bool>& running_flag) {
    std::uint64_t frame_counter_ = 0;
    CE_LOG_INFO("Entering main loop (press Ctrl+C to exit)...");
    while (running_flag.load(std::memory_order_relaxed)) {
        const auto frame_start = clock_type::now();

        on_tick();

        ++frame_counter_;

        // toggle_cycle<CoronaEngineAPI::RenderTag>(600, 300, "RenderTag");
        // toggle_cycle<CoronaEngineAPI::AnimationTag>(720, 360, "AnimationTag");
        // toggle_cycle<CoronaEngineAPI::AudioTag>(840, 420, "AudioTag");
        // toggle_cycle<CoronaEngineAPI::DisplayTag>(960, 480, "DisplayTag");

        {
            const bool has_render_entities = !registry_.storage<CoronaEngineAPI::RenderTag>().empty();
            const bool has_animation_entities = !registry_.storage<CoronaEngineAPI::AnimationTag>().empty();
            const bool has_audio_entities = !registry_.storage<CoronaEngineAPI::AudioTag>().empty();
            const bool has_display_entities = !registry_.storage<CoronaEngineAPI::DisplayTag>().empty();

            update_system(has_render_entities, rendering_running_, *rendering_system_);
            update_system(has_animation_entities, animation_running_, *animation_system_);
            update_system(has_audio_entities, audio_running_, *audio_system_);
            update_system(has_display_entities, display_running_, *display_system_);

            if(frame_counter_ % 600 == 0) {
                CE_LOG_INFO("Frame {}: Systems status - Animation: {}, Rendering: {}, Audio: {}, Display: {}",
                            frame_counter_,
                            animation_running_ ? "Running" : "Paused",
                            rendering_running_ ? "Running" : "Paused",
                            audio_running_ ? "Running" : "Paused",
                            display_running_ ? "Running" : "Paused");
            }
        }

        const auto frame_elapsed = clock_type::now() - frame_start;
        if (frame_elapsed < kFrameDuration) {
            std::this_thread::sleep_for(kFrameDuration - frame_elapsed);
        }
    }
    CE_LOG_INFO("Main loop exited");
}

void RuntimeLoop::shutdown() {
    on_shutdown();
    engine_.stop_systems();
    CE_LOG_INFO("All systems stopped");
}

void RuntimeLoop::build_base_entities() {
    base_entities_.clear();
    base_entities_.reserve(100);

    for (size_t i = 0; i < 100; ++i) {
        auto entity = registry_.create();
        base_entities_.push_back(entity);
        registry_.emplace<CoronaEngineAPI::RenderTag>(entity);
        registry_.emplace<CoronaEngineAPI::AnimationTag>(entity);
        registry_.emplace<CoronaEngineAPI::AudioTag>(entity);
        registry_.emplace<CoronaEngineAPI::DisplayTag>(entity);
    }
}

void RuntimeLoop::on_initialize() {}
void RuntimeLoop::on_shutdown() {}
void RuntimeLoop::on_tick() {}