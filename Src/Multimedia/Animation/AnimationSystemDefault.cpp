//
// Created by 47226 on 2025/9/4.
//

#include "AnimationSystemDefault.hpp"

#include "Core/Logger.h"

namespace Corona
{
    AnimationSystemDefault &AnimationSystemDefault::inst()
    {
        static AnimationSystemDefault inst;
        return inst;
    }
    const char *AnimationSystemDefault::name()
    {
        return "AnimationSystemDefault";
    }
    AnimationSystemDefault::AnimationSystemDefault()
    {
        boneMatrices.resize(100);
    }
    AnimationSystemDefault::~AnimationSystemDefault()
    {
        boneMatrices.clear();
        animationTimeMap.clear();
    }
    void AnimationSystemDefault::start()
    {
        LOG_DEBUG("{} started.", name());
    }
    void AnimationSystemDefault::tick()
    {
        while (!unhandled_data_keys.empty())
        {
            auto id = unhandled_data_keys.front();
            unhandled_data_keys.pop();

            processAnimation(id);
        }

        for (auto id: data_keys)
        {
            processAnimation(id);
        }
    }
    void AnimationSystemDefault::stop()
    {
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

    const std::vector<ktm::fmat4x4> &AnimationSystemDefault::updateBoneAnimation(Corona::Components::Animations *animComp, float dt)
    {
        if (!animComp || animComp->skeletalAnimations.empty())
        {
            static std::vector<ktm::fmat4x4> boneTransforms;
            return boneTransforms;
        }



        return boneMatrices;
    }

    void AnimationSystemDefault::calculateBoneTransform(const Corona::Components::AssimpNodeData *node, ktm::fmat4x4 &parentTransform)
    {
       std::string nodeName = node->name;
       ktm::fmat4x4 nodeTransform = node->transformation;

       Corona::Components::Bone *bone = findBone(nodeName);

       if (bone)
       {
           nodeTransform = updateBone(bone, 0.016f);
       }
    }

    ktm::fmat4x4 AnimationSystemDefault::updateBone(Corona::Components::Bone *bone, float time)
    {
        ktm::fmat4x4 translation = interpolatePosition(bone, time);
        ktm::fmat4x4 rotation = interpolateRotation(bone, time);
        ktm::fmat4x4 scale = interpolateScale(bone, time);
        return translation * rotation * scale;
    }

    Corona::Components::Bone *AnimationSystemDefault::findBone(const std::string &name)
    {
        // for (auto& anim : currentAnimComp->skeletalAnimations)
        // {
        //     auto iter = std::find_if(anim.bones.begin(), anim.bones.end(),
        //                                 [&](const ECS::Components::Bone &bone) {
        //                                     return bone.name == name;
        //                                 });
        //     if (iter != anim.bones.end())
        //         return &(*iter);
        // }
        return nullptr;
    }

    ktm::fmat4x4 AnimationSystemDefault::interpolatePosition(Corona::Components::Bone *bone, float time)
    {
        if (1 == bone->NumPositions)
            return ktm::translate3d(bone->keyPositions[0].position);

        int p0Index = getPositionIndex(bone, time);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(bone->keyPositions[p0Index].time, bone->keyPositions[p1Index].time, time);
        ktm::fvec3 finalPosition = ktm::lerp(bone->keyPositions[p0Index].position, bone->keyPositions[p1Index].position, scaleFactor);
        return ktm::translate3d(finalPosition);
    }

    ktm::fmat4x4 AnimationSystemDefault::interpolateRotation(Corona::Components::Bone *bone, float time)
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

    ktm::fmat4x4 AnimationSystemDefault::interpolateScale(Corona::Components::Bone *bone, float time)
    {
        if (1 == bone->NumScales)
            return ktm::scale3d(bone->keyScales[0].scale);

        int p0Index = getScaleIndex(bone, time);
        int p1Index = p0Index + 1;
        float scaleFactor = getScaleFactor(bone->keyScales[p0Index].time, bone->keyScales[p1Index].time, time);
        ktm::fvec3 finalScale = ktm::lerp(bone->keyScales[p0Index].scale, bone->keyScales[p1Index].scale, scaleFactor);
        return ktm::scale3d(finalScale);
    }

    int AnimationSystemDefault::getPositionIndex(Corona::Components::Bone *bone, float time)
    {
        for (int index = 0; index < bone->NumPositions - 1; ++index)
        {
            if (time < bone->keyPositions[index + 1].time)
                return index;
        }
        return -1;
    }

    int AnimationSystemDefault::getRotationIndex(Corona::Components::Bone *bone, float time)
    {
        for (int index = 0; index < bone->NumRotations - 1; ++index)
        {
            if (time < bone->keyRotations[index + 1].time)
                return index;
        }
        return -1;
    }

    int AnimationSystemDefault::getScaleIndex(Corona::Components::Bone *bone, float time)
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