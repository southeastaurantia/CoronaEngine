// //
// // Created by 47226 on 2025/8/22.
// //
//
// #ifndef CABBAGEFRAMEWORK_ANIMATIONSYSTEM_H
// #define CABBAGEFRAMEWORK_ANIMATIONSYSTEM_H
//
// #include <entt/entt.hpp>
// #include <ktm/ktm.h>
// #include <ECS/Components.h>
//
// #include <thread>
//
// class AnimationSystem
// {
//   public:
//     static constexpr int FPS = 120;
//     static constexpr float MinFrameTime = 1.0f / FPS;
//
//     explicit AnimationSystem(const std::shared_ptr<entt::registry> &registry);
//     ~AnimationSystem();
//
//     void start();
//     void stop();
//
//   private:
//     void loop();
//
//     const std::vector<ktm::fmat4x4> &updateBoneAnimation(ECS::Components::Animations *animComp, float dt);
//     void CalculateBoneTransform(const ECS::Components::AssimpNodeData *node, ktm::fmat4x4 parentTransform);
//     ECS::Components::Bone *FindBone(const std::string &name);
//     ktm::fmat4x4 UpdateBone(ECS::Components::Bone *bone, float time);
//     ktm::fmat4x4 InterpolatePosition(ECS::Components::Bone *bone, float time);
//     ktm::fmat4x4 InterpolateRotation(ECS::Components::Bone *bone, float time);
//     ktm::fmat4x4 InterpolateScale(ECS::Components::Bone *bone, float time);
//     int GetPositionIndex(ECS::Components::Bone *bone, float time);
//     int GetRotationIndex(ECS::Components::Bone *bone, float time);
//     int GetScaleIndex(ECS::Components::Bone *bone, float time);
//     float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float time);
//
//     ECS::Components::Animations *currentAnimComp;
//     std::vector<ktm::fmat4x4> finalBoneMatrices;
//     float currentTime;
//     std::atomic<bool> running;
//     std::thread loopThread;
//     std::shared_ptr<entt::registry> registry;
// };
//
// #endif // CABBAGEFRAMEWORK_ANIMATIONSYSTEM_H
