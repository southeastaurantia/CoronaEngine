//
// Created by 47226 on 2025/8/22.
//

#include "AnimationSystem.h"

#include <ECS/BackBridge.h>

#include <chrono>
#include <iostream>
#include <utility>

#define LOG_DEBUG(message)       \
    if constexpr (LOG_LEVEL < 1) \
    std::cout << std::format("[DEBUG][Anim] {}", message) << std::endl
#define LOG_INFO(message)        \
    if constexpr (LOG_LEVEL < 2) \
    std::cout << std::format("[INFO ][Anim] {}", message) << std::endl
#define LOG_WARNING(message)     \
    if constexpr (LOG_LEVEL < 3) \
    std::cout << std::format("[WARN ][Anim] {}", message) << std::endl
#define LOG_ERROR(message)       \
    if constexpr (LOG_LEVEL < 4) \
    std::cerr << std::format("[ERROR][Anim] {}", message) << std::endl

AnimationSystem::AnimationSystem(const std::shared_ptr<entt::registry> &registry)
    : running(false), registry(registry)
{

    // TODO: BackBridge事件注册

    LOG_INFO("Animation system initialized.");
}

void AnimationSystem::stop()
{
    running.store(false);

    if (loopThread.joinable())
    {
        loopThread.join();
    }
    LOG_INFO("Animation system stopped.");
}

AnimationSystem::~AnimationSystem()
{
    LOG_INFO("Animation system deconstruct.");
}

void AnimationSystem::start()
{
    running.store(true);
    // 启动循环线程
    currentTime = 0.0;
    finalBoneMatrices.reserve(100);
    for (int i = 0; i < 100; i++)
        finalBoneMatrices.push_back(ktm::fmat4x4::from_eye());        
    loopThread = std::thread(&AnimationSystem::loop, this);
    LOG_INFO("Animation system started.");
}

void AnimationSystem::loop()
{
    while (true)
    {
        if (!running.load())
        {
            break;
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        static auto lastFrameTime = startTime;
        auto currentFrameTime = lastFrameTime;
        float dt = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
        lastFrameTime = currentFrameTime;
        BackBridge::anim_dispatcher().update();
        /********** Do Something **********/
        for (auto entity : registry->view<ECS::Components::Animations>())
        {
            auto &animComp = registry->get<ECS::Components::Animations>(entity);
            updateBoneAnimation(&animComp, dt);
        }
        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();

        if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < MinFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MinFrameTime - frameTime) * 1000.0f)));
        }
    }
}

const std::vector<ktm::fmat4x4> &AnimationSystem::updateBoneAnimation(ECS::Components::Animations *animComp, float dt)
{
    currentAnimComp = animComp;

    finalBoneMatrices.clear();

    if (currentAnimComp)
    {
        for (auto &anim : currentAnimComp->skeletalAnimations)
        {
            currentTime += (float)anim.ticksPerSecond * dt;
            currentTime = fmod(currentTime, (float)anim.duration);
            CalculateBoneTransform(&anim.rootNode, ktm::fmat4x4::from_eye());
        }
    }
    return finalBoneMatrices;
}

void AnimationSystem::CalculateBoneTransform(const ECS::Components::AssimpNodeData *node, ktm::fmat4x4 parentTransform)
{
   std::string nodeName = node->name;
   ktm::fmat4x4 nodeTransform = node->transformation;

   ECS::Components::Bone *bone = FindBone(nodeName);

   if (bone)
   {
       nodeTransform = UpdateBone(bone, currentTime);
   }
}

ktm::fmat4x4 AnimationSystem::UpdateBone(ECS::Components::Bone *bone, float time)
{
    ktm::fmat4x4 translation = InterpolatePosition(bone, time);
    ktm::fmat4x4 rotation = InterpolateRotation(bone, time);
    ktm::fmat4x4 scale = InterpolateScale(bone, time);
    return translation * rotation * scale;
}

ECS::Components::Bone *AnimationSystem::FindBone(const std::string &name)
{
    for (auto& anim : currentAnimComp->skeletalAnimations)
    {
        auto iter = std::find_if(anim.bones.begin(), anim.bones.end(),
                                    [&](const ECS::Components::Bone &bone) {
                                        return bone.name == name;
                                    });
        if (iter != anim.bones.end())
            return &(*iter);
    }
    return nullptr;
}

ktm::fmat4x4 AnimationSystem::InterpolatePosition(ECS::Components::Bone *bone, float time)
{
    if (1 == bone->NumPositions)
        return ktm::translate3d(bone->keyPositions[0].position);

    int p0Index = GetPositionIndex(bone, time);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(bone->keyPositions[p0Index].time, bone->keyPositions[p1Index].time, time);
    ktm::fvec3 finalPosition = ktm::lerp(bone->keyPositions[p0Index].position, bone->keyPositions[p1Index].position, scaleFactor);
    return ktm::translate3d(finalPosition);
}

ktm::fmat4x4 AnimationSystem::InterpolateRotation(ECS::Components::Bone *bone, float time)
{
    if (1 == bone->NumRotations)
    {
        ktm::fquat rotation = ktm::normalize(bone->keyRotations[0].rotation);
        return rotation.matrix4x4();
    }

    int p0Index = GetRotationIndex(bone, time);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(bone->keyRotations[p0Index].time, bone->keyRotations[p1Index].time, time);
    ktm::fquat finalRotation = ktm::slerp(bone->keyRotations[p0Index].rotation, bone->keyRotations[p1Index].rotation, scaleFactor);
    finalRotation = ktm::normalize(finalRotation);
    return finalRotation.matrix4x4();
}

ktm::fmat4x4 AnimationSystem::InterpolateScale(ECS::Components::Bone *bone, float time)
{
    if (1 == bone->NumScales)
        return ktm::scale3d(bone->keyScales[0].scale);

    int p0Index = GetScaleIndex(bone, time);
    int p1Index = p0Index + 1;
    float scaleFactor = GetScaleFactor(bone->keyScales[p0Index].time, bone->keyScales[p1Index].time, time);
    ktm::fvec3 finalScale = ktm::lerp(bone->keyScales[p0Index].scale, bone->keyScales[p1Index].scale, scaleFactor);
    return ktm::scale3d(finalScale);
}

int AnimationSystem::GetPositionIndex(ECS::Components::Bone *bone, float time)
{
    for (int index = 0; index < bone->NumPositions - 1; ++index)
    {
        if (time < bone->keyPositions[index + 1].time)
            return index;
    }
    return -1;
}

int AnimationSystem::GetRotationIndex(ECS::Components::Bone *bone, float time)
{
    for (int index = 0; index < bone->NumRotations - 1; ++index)
    {
        if (time < bone->keyRotations[index + 1].time)
            return index;
    }
    return -1;
}

int AnimationSystem::GetScaleIndex(ECS::Components::Bone *bone, float time)
{
    for (int index = 0; index < bone->NumScales - 1; ++index)
    {
        if (time < bone->keyScales[index + 1].time)
            return index;
    }
    return -1;
}

float AnimationSystem::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float time)
{
    float scaleFactor = 0.0f;
    float midWayLength = time - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}