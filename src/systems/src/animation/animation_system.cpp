#include <corona/events/animation_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/animation_system.h>

#include "Animation.h"
#include "Bone.h"
#include "Model.h"

namespace Corona {
const Bone* findBone(const Animation& anim, const std::string& name) {
    for (const Bone& b : anim.m_Bones) {
        if (b.m_Name == name) {
            return &b;
        }
    }
    return nullptr;
}

int getPositionIndex(const Bone& bone, float time) {
    for (int i = 0; i < bone.m_NumPositions - 1; ++i) {
        if (time < bone.m_Positions[i + 1].timeStamp) {
            return i;
        }
    }
    return bone.m_NumPositions - 2;
}

int getRotationIndex(const Bone& bone, float time) {
    for (int i = 0; i < bone.m_NumRotations - 1; ++i) {
        if (time < bone.m_Rotations[i + 1].timeStamp) {
            return i;
        }
    }
    return bone.m_NumRotations - 2;
}

int getScaleIndex(const Bone& bone, float time) {
    for (int i = 0; i < bone.m_NumScales - 1; ++i) {
        if (time < bone.m_Scales[i + 1].timeStamp) {
            return i;
        }
    }
    return bone.m_NumScales - 2;
}

ktm::fmat4x4 interpolatePosition(const Bone& bone, float time) {
    ktm::fvec3 pos;
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;

    if (bone.m_NumPositions == 0) {
        // 返回单位矩阵
        ktm::faffine3d affine;
        affine.translate(pos);
        ktm::fmat4x4 result;
        affine >> result;
        return result;
    }

    if (bone.m_NumPositions == 1) {
        pos = bone.m_Positions[0].position;
    } else {
        const int p0 = getPositionIndex(bone, time);
        const int p1 = p0 + 1;
        const float last = bone.m_Positions[p0].timeStamp;
        const float next = bone.m_Positions[p1].timeStamp;
        const float factor = (time - last) / (next - last);
        pos = ktm::lerp(bone.m_Positions[p0].position, bone.m_Positions[p1].position, factor);
    }

    ktm::faffine3d affine;
    affine.translate(pos);
    ktm::fmat4x4 result;
    affine >> result;
    return result;
}

ktm::fmat4x4 interpolateRotation(const Bone& bone, float time) {
    ktm::fquat rot = ktm::fquat::identity();

    if (bone.m_NumRotations == 0) {
        ktm::faffine3d affine;
        affine.rotate(rot);
        ktm::fmat4x4 result;
        affine >> result;
        return result;
    }

    if (bone.m_NumRotations == 1) {
        rot = ktm::normalize(bone.m_Rotations[0].orientation);
    } else {
        const int r0 = getRotationIndex(bone, time);
        const int r1 = r0 + 1;
        const float last = bone.m_Rotations[r0].timeStamp;
        const float next = bone.m_Rotations[r1].timeStamp;
        const float factor = (time - last) / (next - last);
        rot = ktm::slerp(bone.m_Rotations[r0].orientation, bone.m_Rotations[r1].orientation, factor);
        rot = ktm::normalize(rot);
    }

    ktm::faffine3d affine;
    affine.rotate(rot);
    ktm::fmat4x4 result;
    affine >> result;
    return result;
}

ktm::fmat4x4 interpolateScale(const Bone& bone, float time) {
    ktm::fvec3 sc;
    sc.x = 1.0f;
    sc.y = 1.0f;
    sc.z = 1.0f;

    if (bone.m_NumScales == 0) {
        ktm::faffine3d affine;
        affine.scale(sc);
        ktm::fmat4x4 result;
        affine >> result;
        return result;
    }

    if (bone.m_NumScales == 1) {
        sc = bone.m_Scales[0].scale;
    } else {
        const int s0 = getScaleIndex(bone, time);
        const int s1 = s0 + 1;
        const float last = bone.m_Scales[s0].timeStamp;
        const float next = bone.m_Scales[s1].timeStamp;
        const float factor = (time - last) / (next - last);
        sc = ktm::lerp(bone.m_Scales[s0].scale, bone.m_Scales[s1].scale, factor);
    }

    ktm::faffine3d affine;
    affine.scale(sc);
    ktm::fmat4x4 result;
    affine >> result;
    return result;
}

void calculate_bone_transform(
    const std::shared_ptr<Model>& model,
    float current_time,
    const Animation& anim,
    const Animation::AssimpNodeData& node,
    const ktm::fmat4x4& parent,
    std::vector<ktm::fmat4x4>& outBones) {
    ktm::fmat4x4 nodeTransform = node.transformation;

    if (const Bone* b = findBone(anim, node.name)) {
        const ktm::fmat4x4 T = interpolatePosition(*b, current_time);
        const ktm::fmat4x4 R = interpolateRotation(*b, current_time);
        const ktm::fmat4x4 S = interpolateScale(*b, current_time);
        nodeTransform = T * R * S;
    }

    const ktm::fmat4x4 global = parent * nodeTransform;

    if (auto it = model->m_BoneInfoMap.find(node.name); it != model->m_BoneInfoMap.end()) {
        const int index = it->second->ID;
        const ktm::fmat4x4& offset = it->second->OffsetMatrix;
        if (index >= 0 && static_cast<size_t>(index) < outBones.size()) {
            outBones[index] = global * offset;
        }
    }

    for (const auto& child : node.children) {
        calculate_bone_transform(model, current_time, anim, child, global, outBones);
    }
}
} // namespace Corona

namespace Corona::Systems {
bool AnimationSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("AnimationSystem: Initializing...");
    return true;
}

void AnimationSystem::update() {
    update_animation();
}

void AnimationSystem::update_animation() {
    const float dt = 0.016f;

    // 正确使用 for_each_write 来修改 AnimationState
    SharedDataHub::instance().animation_state_storage().for_each_write([&](AnimationState& state) {
        update_animation_state(state, dt);
    });
}

void AnimationSystem::update_animation_state(AnimationState& state, float dt) {
    if (state.model_handle == 0 || !state.active) {
        return;
    }

    std::cout << "Updating animation for model handle: " << state.model_handle << std::endl;
    bool should_update = false;
    const Animation* current_anim = nullptr;
    std::shared_ptr<Model> model_ptr;

    bool read_success = SharedDataHub::instance().model_storage().read(
        state.model_handle,
        [&](const std::shared_ptr<Model>& model) {
            model_ptr = model;
            if (!model) {
                return;
            }
            if (const auto anim_count = model->skeletalAnimations.size();
                anim_count > 0 && state.animation_index < anim_count) {
                current_anim = &model->skeletalAnimations[state.animation_index];
                should_update = true;
            }
        });

    if (!read_success || !should_update || !current_anim || !model_ptr) {
        return;
    }

    // 更新动画时间
    state.current_time += static_cast<float>(current_anim->m_TicksPerSecond) * dt;
    if (const auto duration = static_cast<float>(current_anim->m_Duration); duration > 0.0f) {
        state.current_time = std::fmod(state.current_time, duration);
    }

    // 计算骨骼变换矩阵
    const size_t bone_count = model_ptr->m_BoneInfoMap.size();
    if (bone_count == 0) {
        return;
    }

    std::vector<ktm::fmat4x4> bone_matrices(bone_count, ktm::fmat4x4::from_eye());
    const ktm::fmat4x4 identity = ktm::fmat4x4::from_eye();

    calculate_bone_transform(
        model_ptr,
        state.current_time,
        *current_anim,
        current_anim->m_RootNode,
        identity,
        bone_matrices);

    const std::uintptr_t target_model_handle = state.model_handle;

    SharedDataHub::instance().model_device_storage().for_each_write([&](ModelDevice& device) {
        if (device.model_handle != target_model_handle) {
            return;
        }
        if (!bone_matrices.empty()) {
            device.bone_matrix_buffer.copyFromData(
                bone_matrices.data(),
                bone_matrices.size() * sizeof(ktm::fmat4x4));
        }
    });
}

void AnimationSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AnimationSystem: Shutting down...");
}
} // namespace Corona::Systems