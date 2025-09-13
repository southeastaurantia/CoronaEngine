#pragma once

#include "Core/Engine/ThreadedSystem.h"
#include "Resource/AnimationState.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

// 前置声明，避免在头文件引入重量级头
namespace Corona
{
    class Animation;
    class Bone;
} // namespace Corona

namespace Corona
{
    class AnimationSystem final : public ThreadedSystem
    {
      public:
        AnimationSystem();
        // 向系统注册/取消关注的 AnimationState id（通过命令队列串行修改）
        static void WatchState(uint64_t id);
        static void UnwatchState(uint64_t id);
        static void ClearWatched();

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        std::vector<float> boneMatrices_; // 占位：后续替换为矩阵类型
        float currentTime_ = 0.0f;
        std::unordered_map<uint64_t, float> animationTime_{};
        std::unordered_set<uint64_t> data_keys_{};
        void processAnimation(uint64_t id);
        // 关注的动画状态 id 集合
        void updateAnimationState(AnimationState &state, float dt);
    };
} // namespace Corona
