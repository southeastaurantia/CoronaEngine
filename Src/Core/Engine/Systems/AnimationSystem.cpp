#include "AnimationSystem.h"
#include "Core/Engine/Engine.h"
#include "Resource/Animation.h"
#include "Resource/Bone.h"
#include "Resource/Model.h"

#include <cmath>

namespace
{
    using namespace Corona;

    // 在本 cpp 内部隐藏的帮助函数，避免在头文件暴露重量类型
    static const Bone *findBone(const Animation &anim, const std::string &name)
    {
        for (const Bone &b : anim.m_Bones)
            if (b.m_Name == name)
                return &b;
        return nullptr;
    }

    static int getPositionIndex(const Bone &bone, float time)
    {
        for (int i = 0; i < bone.m_NumPositions - 1; ++i)
            if (time < bone.m_Positions[i + 1].timeStamp)
                return i;
        return bone.m_NumPositions - 2;
    }
    static int getRotationIndex(const Bone &bone, float time)
    {
        for (int i = 0; i < bone.m_NumRotations - 1; ++i)
            if (time < bone.m_Rotations[i + 1].timeStamp)
                return i;
        return bone.m_NumRotations - 2;
    }
    static int getScaleIndex(const Bone &bone, float time)
    {
        for (int i = 0; i < bone.m_NumScales - 1; ++i)
            if (time < bone.m_Scales[i + 1].timeStamp)
                return i;
        return bone.m_NumScales - 2;
    }

    static ktm::fmat4x4 interpolatePosition(const Bone &bone, float time)
    {
        if (bone.m_NumPositions <= 0)
            return ktm::translate3d(ktm::fvec3(0, 0, 0));
        if (bone.m_NumPositions == 1)
            return ktm::translate3d(bone.m_Positions[0].position);
        const int p0 = getPositionIndex(bone, time);
        const int p1 = p0 + 1;
        const float last = bone.m_Positions[p0].timeStamp;
        const float next = bone.m_Positions[p1].timeStamp;
        const float factor = (time - last) / (next - last);
        const ktm::fvec3 pos = ktm::lerp(bone.m_Positions[p0].position, bone.m_Positions[p1].position, factor);
        return ktm::translate3d(pos);
    }
    static ktm::fmat4x4 interpolateRotation(const Bone &bone, float time)
    {
        if (bone.m_NumRotations <= 0)
            return ktm::fquat(0, 0, 0, 1).matrix4x4();
        if (bone.m_NumRotations == 1)
            return ktm::normalize(bone.m_Rotations[0].orientation).matrix4x4();
        const int r0 = getRotationIndex(bone, time);
        const int r1 = r0 + 1;
        const float last = bone.m_Rotations[r0].timeStamp;
        const float next = bone.m_Rotations[r1].timeStamp;
        const float factor = (time - last) / (next - last);
        ktm::fquat rot = ktm::slerp(bone.m_Rotations[r0].orientation, bone.m_Rotations[r1].orientation, factor);
        rot = ktm::normalize(rot);
        return rot.matrix4x4();
    }
    static ktm::fmat4x4 interpolateScale(const Bone &bone, float time)
    {
        if (bone.m_NumScales <= 0)
            return ktm::scale3d(ktm::fvec3(1, 1, 1));
        if (bone.m_NumScales == 1)
            return ktm::scale3d(bone.m_Scales[0].scale);
        const int s0 = getScaleIndex(bone, time);
        const int s1 = s0 + 1;
        const float last = bone.m_Scales[s0].timeStamp;
        const float next = bone.m_Scales[s1].timeStamp;
        const float factor = (time - last) / (next - last);
        const ktm::fvec3 sc = ktm::lerp(bone.m_Scales[s0].scale, bone.m_Scales[s1].scale, factor);
        return ktm::scale3d(sc);
    }

    static void calculateBoneTransform(const AnimationState &state,
                                       const Animation &anim,
                                       const Animation::AssimpNodeData &node,
                                       const ktm::fmat4x4 &parent,
                                       std::vector<ktm::fmat4x4> &outBones)
    {
        ktm::fmat4x4 nodeTransform = node.transformation;
        if (const Bone *b = findBone(anim, node.name))
        {
            const ktm::fmat4x4 T = interpolatePosition(*b, state.currentTime);
            const ktm::fmat4x4 R = interpolateRotation(*b, state.currentTime);
            const ktm::fmat4x4 S = interpolateScale(*b, state.currentTime);
            nodeTransform = T * R * S;
        }
        const ktm::fmat4x4 global = parent * nodeTransform;
        if (auto it = state.model->m_BoneInfoMap.find(node.name); it != state.model->m_BoneInfoMap.end())
        {
            const int index = it->second->ID;
            const ktm::fmat4x4 &offset = it->second->OffsetMatrix;
            if (index >= 0 && static_cast<size_t>(index) < outBones.size())
                outBones[index] = global * offset;
        }
        for (const auto &child : node.children)
            calculateBoneTransform(state, anim, child, global, outBones);
    }
} // namespace

using namespace Corona;

AnimationSystem::AnimationSystem()
    : ThreadedSystem("AnimationSystem")
{
    // 为本系统注册命令队列
    Engine::Instance().AddQueue(name(), std::make_unique<SafeCommandQueue>());
}

void AnimationSystem::onStart()
{
    currentTime_ = 0.0f;
}

void AnimationSystem::onTick()
{
    auto &rq = Engine::Instance().GetQueue(name());
    int spun = 0;
    while (spun < 100 && !rq.empty())
    {
        if (!rq.try_execute())
            continue;
        ++spun;
    }
    // 遍历关注的 AnimationState 并推进
    auto &cache = Engine::Instance().Cache<AnimationState>();
    cache.safe_loop_foreach(data_keys_, [&](std::shared_ptr<AnimationState> st) {
        if (!st || !st->model)
            return;
        updateAnimationState(*st, 1.0f / 120.0f); // 与系统线程目标帧率一致
    });
}

void AnimationSystem::onStop()
{
    boneMatrices_.clear();
    animationTime_.clear();
}

void AnimationSystem::processAnimation(uint64_t /*id*/)
{
}

// static
void AnimationSystem::WatchState(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("AnimationSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<AnimationSystem>()]() mutable {
        sys.data_keys_.insert(id);
    });
}

// static
void AnimationSystem::UnwatchState(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("AnimationSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<AnimationSystem>()]() mutable {
        sys.data_keys_.erase(id);
    });
}

// static
void AnimationSystem::ClearWatched()
{
    auto &q = Engine::Instance().GetQueue("AnimationSystem");
    q.enqueue([&sys = Engine::Instance().GetSystem<AnimationSystem>()]() mutable {
        sys.data_keys_.clear();
    });
}

void AnimationSystem::updateAnimationState(AnimationState &state, float dt)
{
    if (!state.model)
        return;
    const auto animCount = state.model->skeletalAnimations.size();
    if (animCount == 0 || state.animationIndex >= animCount)
        return;

    const Animation &anim = state.model->skeletalAnimations[state.animationIndex];
    // 推进时间（anim.m_TicksPerSecond 为动画时钟刻度）
    state.currentTime += static_cast<float>(anim.m_TicksPerSecond) * dt;
    const float duration = static_cast<float>(anim.m_Duration);
    if (duration > 0.0f)
        state.currentTime = fmod(state.currentTime, duration);

    // 准备输出骨矩阵数组
    const size_t boneCount = state.model->m_BoneInfoMap.size();
    state.bones.resize(boneCount, ktm::fmat4x4::from_eye());

    const ktm::fmat4x4 identity = ktm::fmat4x4::from_eye();
    // 从根开始递归计算
    calculateBoneTransform(state, anim, anim.m_RootNode, identity, state.bones);
}
