#include <corona/events/animation_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/animation_system.h>

namespace Corona::Systems {

bool AnimationSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("AnimationSystem: Initializing...");
    return true;
}

void AnimationSystem::update() {

}

void AnimationSystem::update_animation_state() {
    // if (!state.model) {
    //     return;
    // }
    // const auto animCount = state.model->skeletalAnimations.size();
    // if (animCount == 0 || state.animationIndex >= animCount) {
    //     return;
    // }
    //
    // const Animation& anim = state.model->skeletalAnimations[state.animationIndex];
    // // 推进时间（anim.m_TicksPerSecond 为动画时钟刻度）
    // state.currentTime += static_cast<float>(anim.m_TicksPerSecond) * dt;
    // const auto duration = static_cast<float>(anim.m_Duration);
    // if (duration > 0.0f) {
    //     state.currentTime = fmod(state.currentTime, duration);
    // }
    //
    // // 准备输出骨矩阵数组
    // const size_t boneCount = state.model->m_BoneInfoMap.size();
    // state.bones.resize(boneCount, ktm::fmat4x4::from_eye());
    //
    // const ktm::fmat4x4 identity = ktm::fmat4x4::from_eye();
    // // 从根开始递归计算
    // calculateBoneTransform(state, anim, anim.m_RootNode, identity, state.bones);

    // if (!state.model->bonesMatrixBuffer) {
    //     state.model->bonesMatrixBuffer = HardwareBuffer(state.bones, BufferUsage::StorageBuffer);
    // } else {
    //     state.model->bonesMatrixBuffer.copyFromData(state.bones.data(), state.bones.size() * sizeof(ktm::fmat4x4));
    // }
}

void AnimationSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AnimationSystem: Shutting down...");
}

}  // namespace Corona::Systems