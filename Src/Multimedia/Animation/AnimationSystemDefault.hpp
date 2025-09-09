//
// Created by 47226 on 2025/9/4.
//

#ifndef CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#define CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#include "Core/SafeDataCache.h"
#include "Engine.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

#include <unordered_set>

namespace Corona
{

    class AnimationSystemDefault final : public BaseMultimediaSystem
    {
      public:

        const char *name() override;

      public:
        explicit AnimationSystemDefault();
        ~AnimationSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::thread animationThread;
        std::atomic<bool> running;
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key
        std::unordered_map<DataCache::id_type, float> animationTimeMap;
        std::vector<ktm::fmat4x4> boneMatrices;
        float currentTime;

        void processAnimation(DataCache::id_type id);
        const std::vector<ktm::fmat4x4> &updateBoneAnimation(const Corona::Components::Animations *animComp, float dt);
        void calculateBoneTransform(const Corona::Components::Animations *animComp, ktm::fmat4x4 &parentTransform);
        Corona::Components::Bone *findBone(const Corona::Components::Animations *animComp, const std::string &name);
        ktm::fmat4x4 updateBone(const Corona::Components::Bone *bone, float time);
        ktm::fmat4x4 interpolatePosition(const Corona::Components::Bone *bone, float time);
        ktm::fmat4x4 interpolateRotation(const Corona::Components::Bone *bone, float time);
        ktm::fmat4x4 interpolateScale(const Corona::Components::Bone *bone, float time);
        int getPositionIndex(const Corona::Components::Bone *bone, float time);
        int getRotationIndex(const Corona::Components::Bone *bone, float time);
        int getScaleIndex(const Corona::Components::Bone *bone, float time);
        float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float time);
    };

} // namespace Corona

#endif // CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
