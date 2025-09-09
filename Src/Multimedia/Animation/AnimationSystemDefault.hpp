//
// Created by 47226 on 2025/9/4.
//

#ifndef CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#define CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#include "Engine.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

#include <unordered_set>

namespace Corona
{

    class AnimationSystemDefault final : public BaseMultimediaSystem
    {
      public:
        static AnimationSystemDefault &inst();

        const char *name() override;

      protected:
        explicit AnimationSystemDefault();
        ~AnimationSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key
        std::queue<DataCache::id_type> unhandled_data_keys; // 当前帧未处理的key
        std::unordered_map<DataCache::id_type, float> animationTimeMap;
        std::vector<ktm::fmat4x4> boneMatrices;

        void processAnimation(DataCache::id_type id);
        const std::vector<ktm::fmat4x4> &updateBoneAnimation(Corona::Components::Animations *animComp, float dt);
        void calculateBoneTransform(const Corona::Components::AssimpNodeData *node, ktm::fmat4x4 &transform);
        Corona::Components::Bone *findBone(const std::string &name);
        ktm::fmat4x4 updateBone(Corona::Components::Bone *bone, float time);
        ktm::fmat4x4 interpolatePosition(Corona::Components::Bone *bone, float time);
        ktm::fmat4x4 interpolateRotation(Corona::Components::Bone *bone, float time);
        ktm::fmat4x4 interpolateScale(Corona::Components::Bone *bone, float time);
        int getPositionIndex(Corona::Components::Bone *bone, float time);
        int getRotationIndex(Corona::Components::Bone *bone, float time);
        int getScaleIndex(Corona::Components::Bone *bone, float time);
        float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float time);
    };

} // namespace Corona

#endif // CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
