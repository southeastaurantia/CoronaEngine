#pragma once

#include <Engine.h>
#include <corona_logger.h>

#include <atomic>
#include <entt/entt.hpp>
#include <vector>

namespace Corona {
class AnimationSystem;
class RenderingSystem;
class AudioSystem;
class DisplaySystem;
}  // namespace Corona

class RuntimeLoop {
   public:
    explicit RuntimeLoop(Corona::Engine& engine);

    void initialize();
    void run(std::atomic<bool>& running_flag);
    void shutdown();

   protected:
    virtual void on_initialize();
    virtual void on_shutdown();
    virtual void on_tick();

   private:
    template <typename Tag>
    void toggle_cycle(int cycle_frames, int add_offset, const char* label);

    template <typename System>
    static void update_system(bool should_run, bool& running_state, System& system);

    void build_base_entities();

    Corona::Engine& engine_;
    entt::registry registry_{};
    std::vector<entt::entity> base_entities_{};
    int frame_counter_ = 0;

    Corona::AnimationSystem* animation_system_ = nullptr;
    Corona::RenderingSystem* rendering_system_ = nullptr;
    Corona::AudioSystem* audio_system_ = nullptr;
    Corona::DisplaySystem* display_system_ = nullptr;

    bool animation_running_ = false;
    bool rendering_running_ = false;
    bool audio_running_ = false;
    bool display_running_ = false;
};

// -----------------------------------------------------------------------------
// Template implementation
// -----------------------------------------------------------------------------

template <typename Tag>
void RuntimeLoop::toggle_cycle(int cycle_frames, int add_offset, const char* label) {
    const int phase = frame_counter_ % cycle_frames;

    if (phase == 0) {
        size_t removed = 0;
        for (auto entity : base_entities_) {
            if (registry_.any_of<Tag>(entity)) {
                registry_.remove<Tag>(entity);
                ++removed;
            }
        }
        if (removed > 0) {
            CE_LOG_DEBUG("{} removed from {} entities", label, static_cast<uint32_t>(removed));
        }
    } else if (phase == add_offset) {
        size_t added = 0;
        for (auto entity : base_entities_) {
            if (!registry_.any_of<Tag>(entity)) {
                registry_.emplace<Tag>(entity);
                ++added;
            }
        }
        if (added > 0) {
            CE_LOG_DEBUG("{} added to {} entities", label, static_cast<uint32_t>(added));
        }
    }
}

template <typename System>
void RuntimeLoop::update_system(bool should_run, bool& running_state, System& system) {
    if (should_run == running_state) {
        return;
    }
    if (should_run) {
        system.start();
    } else {
        system.stop();
    }
    running_state = should_run;
}
