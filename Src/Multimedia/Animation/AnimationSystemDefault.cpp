//
// Created by 47226 on 2025/9/4.
//

#include "AnimationSystemDefault.hpp"

#include "Core/Logger.h"

namespace Corona
{
    const char *AnimationSystemDefault::name()
    {
        return "AnimationSystemDefault";
    }
    AnimationSystemDefault::AnimationSystemDefault()
        : running(false)
        , currentTime(0.0f)
        , boneMatrices(100,{})
        , animationTimeMap({})
    {
    }
    AnimationSystemDefault::~AnimationSystemDefault()
    {
        LOG_DEBUG("AnimationSystemDefault::~AnimationSystemDefault()");
        boneMatrices.clear();
        animationTimeMap.clear();
    }
    void AnimationSystemDefault::start()
    {
        currentTime = 0.0f;
        for (int i = 0; i < 100; i++)
            boneMatrices.push_back(ktm::fmat4x4::from_eye());
        running.store(true);
        constexpr int64_t TARGET_FRAME_TIME_US = 1000000 / 120;

        animationThread = std::thread([&] {
            while (running.load())
            {
                auto begin = std::chrono::high_resolution_clock::now();
                tick();
                auto end = std::chrono::high_resolution_clock::now();
                auto frameTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
                auto waitTimeUs = TARGET_FRAME_TIME_US - frameTimeUs;

                if (waitTimeUs > 0)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(waitTimeUs));
                }
            }
        });
        LOG_DEBUG("{} started.", name());
    }
    void AnimationSystemDefault::tick()
    {

    }
    void AnimationSystemDefault::stop()
    {
        running.store(false);
        if (animationThread.joinable())
        {
            animationThread.join();
        }
        LOG_DEBUG("{} stopped.", name());
    }

    void AnimationSystemDefault::processAnimation(DataCache::id_type id)
    {
        auto &cache = Engine::inst().data_cache();
        tbb::concurrent_hash_map<DataCache::id_type, std::shared_ptr<Corona::Components::Animations>>::accessor it;

        if (cache.animations.find(it, id))
        {
            auto animations = it->second;
            if (animations)
            {
                const auto& boneTransforms = updateBoneAnimation(animations.get(), 0.016f);
            }
        }
    }

    const std::vector<ktm::fmat4x4> &AnimationSystemDefault::updateBoneAnimation(const Corona::Components::Animations *animComp, float dt)
    {
        boneMatrices.clear();

        if (animComp)
        {
            for (auto &anim : animComp->skeletalAnimations)
            {
                currentTime += static_cast<float>(anim.ticksPerSecond) * dt;
                currentTime = fmod(currentTime, static_cast<float>(anim.duration));
                ktm::fmat4x4 identityTransform = ktm::fmat4x4::from_eye();
                calculateBoneTransform(animComp, identityTransform);
            }
        }
        return boneMatrices;
    }

    void AnimationSystemDefault::calculateBoneTransform(const Corona::Components::Animations *animComp, ktm::fmat4x4 &parentTransform)
    {
       std::string nodeName = animComp->skeletalAnimations[0].rootNode.name;
       ktm::fmat4x4 nodeTransform = animComp->skeletalAnimations[0].rootNode.transformation;

       if (Corona::Components::Bone *bone = findBone(animComp, nodeName))
       {
           nodeTransform = updateBone(bone, 0.016f);
       }

        ktm::fmat4x4 globalTransform = parentTransform * nodeTransform;

        auto boneInfoMap = animComp->boneInfoMap;
        if (boneInfoMap.contains(nodeName))
        {
            int index = boneInfoMap[nodeName].id;
            ktm::fmat4x4 offset = boneInfoMap[nodeName].offsetMatrix;
            boneMatrices[index] = globalTransform * offset;
        }

        for (int i = 0; i< animComp->skeletalAnimations[0].rootNode.children.size(); i++)
        {
            calculateBoneTransform(animComp, globalTransform);
        }

    }

    ktm::fmat4x4 AnimationSystemDefault::updateBone(const Corona::Components::Bone *bone, float time)
    {
        ktm::fmat4x4 translation = interpolatePosition(bone, time);
        ktm::fmat4x4 rotation = interpolateRotation(bone, time);
        ktm::fmat4x4 scale = interpolateScale(bone, time);
        return translation * rotation * scale;
    }

    Corona::Components::Bone *AnimationSystemDefault::findBone(const Corona::Components::Animations *animComp, const std::string &name)
    {
        for (auto anim : animComp->skeletalAnimations)
        {
            auto iter = std::ranges::find_if(anim.bones, [&](const Corona::Components::Bone &bone) {
                return bone.name == name;
            });
            if (iter != anim.bones.end())
                return &(*iter);
        }
        return nullptr;
    }

    ktm::fmat4x4 AnimationSystemDefault::interpolatePosition(const Corona::Components::Bone *bone, float time)
    {
        if (1 == bone->NumPositions)
            return ktm::translate3d(bone->keyPositions[0].position);

        int p0Index = getPositionIndex(bone, time);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(bone->keyPositions[p0Index].time, bone->keyPositions[p1Index].time, time);
        ktm::fvec3 finalPosition = ktm::lerp(bone->keyPositions[p0Index].position, bone->keyPositions[p1Index].position, scaleFactor);
        return ktm::translate3d(finalPosition);
    }

    ktm::fmat4x4 AnimationSystemDefault::interpolateRotation(const Corona::Components::Bone *bone, float time)
    {
        if (1 == bone->NumRotations)
        {
            ktm::fquat rotation = ktm::normalize(bone->keyRotations[0].rotation);
            return rotation.matrix4x4();
        }

        int p0Index = getRotationIndex(bone, time);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(bone->keyRotations[p0Index].time, bone->keyRotations[p1Index].time, time);
        ktm::fquat finalRotation = ktm::slerp(bone->keyRotations[p0Index].rotation, bone->keyRotations[p1Index].rotation, scaleFactor);
        finalRotation = ktm::normalize(finalRotation);
        return finalRotation.matrix4x4();
    }

    ktm::fmat4x4 AnimationSystemDefault::interpolateScale(const Corona::Components::Bone *bone, float time)
    {
        if (1 == bone->NumScales)
            return ktm::scale3d(bone->keyScales[0].scale);

        int p0Index = getScaleIndex(bone, time);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(bone->keyScales[p0Index].time, bone->keyScales[p1Index].time, time);
        ktm::fvec3 finalScale = ktm::lerp(bone->keyScales[p0Index].scale, bone->keyScales[p1Index].scale, scaleFactor);
        return ktm::scale3d(finalScale);
    }

    int AnimationSystemDefault::getPositionIndex(const Corona::Components::Bone *bone, float time)
    {
        for (int index = 0; index < bone->NumPositions - 1; ++index)
        {
            if (time < bone->keyPositions[index + 1].time)
                return index;
        }
        return -1;
    }

    int AnimationSystemDefault::getRotationIndex(const Corona::Components::Bone *bone, float time)
    {
        for (int index = 0; index < bone->NumRotations - 1; ++index)
        {
            if (time < bone->keyRotations[index + 1].time)
                return index;
        }
        return -1;
    }

    int AnimationSystemDefault::getScaleIndex(const Corona::Components::Bone *bone, float time)
    {
        for (int index = 0; index < bone->NumScales - 1; ++index)
        {
            if (time < bone->keyScales[index + 1].time)
                return index;
        }
        return -1;
    }

    float AnimationSystemDefault::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float time)
    {
        float scaleFactor = 0.0f;
        float midWayLength = time - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;
        return scaleFactor;
    }

} // namespace CoronaEngine