#include <corona/events/engine_events.h>
#include <corona/events/kinematics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/kinematics/kinematics_system.h>
#include <ktm/ktm.h>

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
}  // namespace Corona

namespace Corona::Systems {
bool KinematicsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("AnimationSystem: Initializing...");
    return true;
}

void KinematicsSystem::update() {
    update_animation();
}

void KinematicsSystem::update_animation() {
    const float dt = 0.016f;

    // 1. 遍历所有动画控制器，更新动画时间
    SharedDataHub::instance().animation_controller_storage().for_each_write([&](AnimationState& state) {
        if (!state.active) {
            return;
        }
        state.current_time += state.playback_speed * dt;
    });

    // 2. 遍历所有 KinematicsDevice，计算并写入骨骼矩阵
    SharedDataHub::instance().kinematics_storage().for_each_read([&](const KinematicsDevice& kine) {
        if (kine.animation_controller_handle == 0 || kine.geometry_handle == 0) {
            return;
        }

        // 读取动画状态
        AnimationState anim_state{};
        bool has_state = SharedDataHub::instance().animation_controller_storage().read(kine.animation_controller_handle, [&](const AnimationState& s) {
            anim_state = s;
        });

        if (!has_state || !anim_state.active) {
            return;
        }

        std::shared_ptr<Model> model_ptr;
        const Animation* current_anim = nullptr;

        bool has_model = SharedDataHub::instance().geometry_storage().read(kine.geometry_handle, [&](const GeometryDevice& geom) {
            SharedDataHub::instance().model_resource_storage().read(geom.model_resource_handle, [&](const ModelResource& res) {
                model_ptr = res.model_ptr;
                if (model_ptr && !model_ptr->skeletalAnimations.empty() &&
                    anim_state.animation_index < model_ptr->skeletalAnimations.size()) {
                    current_anim = &model_ptr->skeletalAnimations[anim_state.animation_index];
                }
            });
        });

        if (!has_model || !model_ptr || !current_anim) {
            return;
        }

        // 计算动画当前时间（循环）
        float current_time = anim_state.current_time;
        if (current_anim->m_TicksPerSecond > 0.0) {
            current_time *= static_cast<float>(current_anim->m_TicksPerSecond);
        }
        if (current_anim->m_Duration > 0.0) {
            current_time = std::fmod(current_time, static_cast<float>(current_anim->m_Duration));
        }

        // 计算骨骼矩阵
        const size_t bone_count = model_ptr->m_BoneInfoMap.size();
        if (bone_count == 0) {
            return;
        }

        std::vector<ktm::fmat4x4> bone_matrices(bone_count, ktm::fmat4x4::from_eye());
        const ktm::fmat4x4 identity = ktm::fmat4x4::from_eye();

        calculate_bone_transform(
            model_ptr,
            current_time,
            *current_anim,
            current_anim->m_RootNode,
            identity,
            bone_matrices);

        // 写入 skinning 的骨骼矩阵缓冲
        if (kine.skinning_handle != 0) {
            SharedDataHub::instance().skinning_storage().write(kine.skinning_handle, [&](SkinningDevice& skin) {
                if (!bone_matrices.empty()) {
                    skin.bone_matrix_buffer.copyFromData(
                        bone_matrices.data(),
                        bone_matrices.size() * sizeof(ktm::fmat4x4));
                }
            });
        }
    });
}

void KinematicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AnimationSystem: Shutting down...");
}
}  // namespace Corona::Systems