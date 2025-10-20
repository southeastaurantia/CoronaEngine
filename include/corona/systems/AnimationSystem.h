#pragma once

#include <corona/interfaces/Services.h>
#include <corona/interfaces/ThreadedSystem.h>

#include <cstdint>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "AnimationState.h"

// 前置声明，避免在头文件引入重量级头
namespace Corona {
class Animation;
class Bone;
class Model;
}  // namespace Corona

namespace Corona {
class AnimationSystem final : public ThreadedSystem {
   public:
    AnimationSystem();
    void configure(const Interfaces::SystemContext& context) override;
    // 向系统注册/取消关注的 AnimationState id（通过命令队列串行修改）
    void watch_state(uint64_t id);
    void unwatch_state(uint64_t id);
    void watch_model(uint64_t id);
    void unwatch_model(uint64_t id);
    void clear_watched();

   protected:
    void onStart() override;
    void onTick() override;
    void onStop() override;

   private:
    std::vector<float> boneMatrices_;  // 占位：后续替换为矩阵类型
    float playback_speed = 1.0f;       // 播放速度倍数
    std::unordered_map<uint64_t, float> animationTime_{};
    std::unordered_set<uint64_t> state_cache_keys_{};
    std::unordered_set<uint64_t> model_cache_keys_{};
    std::unordered_set<uint64_t> other_model_cache_keys_{};
    std::set<Model*> collisionActors_{};
    static void process_animation(uint64_t id);
    static void update_animation_state(AnimationState& state, float dt);
    void send_collision_event();
    void update_physics(Model& model);
    int frame_count_ = 0;
    std::shared_ptr<Interfaces::IResourceService> resource_service_{};
    std::shared_ptr<Interfaces::ICommandScheduler> scheduler_{};
    Interfaces::ICommandScheduler::QueueHandle system_queue_handle_{};
};
}  // namespace Corona
