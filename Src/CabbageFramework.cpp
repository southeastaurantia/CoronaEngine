#include "CabbageFramework.h"

#include <ECS/Components.h>
#include <ECS/Events.hpp>
#include <ECS/Global.h>

#include <format>
#include <iostream>

// Actor实现
CabbageFramework::Actor::Actor(const Scene &scene, const std::string &path)
    : actorID(static_cast<uint64_t>(ECS::Global::get().registry->create())) // 创建Actor实体
{
    // Actor实体添加组件
    entt::entity actor = static_cast<entt::entity>(actorID);
    ECS::Global::get().registry->emplace<ECS::Components::ActorPose>(actor, std::move(ECS::Components::ActorPose{}));
    ECS::Global::get().registry->emplace<ECS::Components::BoneMatrixDevice>(actor, std::move(ECS::Components::BoneMatrixDevice{}));
    ECS::Global::get().registry->emplace<ECS::Components::BoneMatrixHost>(actor, std::move(ECS::Components::BoneMatrixHost{}));
    
    entt::entity modelEntity = ECS::Global::get().resourceMgr->LoadModel(path);
    ECS::Global::get().registry->emplace<ECS::Components::Model>(actor, std::move(ECS::Components::Model{.model = entt::null}));
    // 发送事件给ECS系统
    // ECS::Global::get().dispatcher->enqueue<ECS::Events::CreateActorEntity>(std::move(ECS::Events::CreateActorEntity{
    //     .scene = static_cast<entt::entity>(scene.sceneID),
    //     .actor = actor,
    //     .path = path}));
    std::cout << std::format("Actor created with ID: {}\n", actorID);
}

CabbageFramework::Actor::~Actor()
{
    entt::entity actor = static_cast<entt::entity>(actorID);
    ECS::Global::get().dispatcher->enqueue<ECS::Events::DestroyActorEntity>(std::move(ECS::Events::DestroyActorEntity{
        .actor = actor}));

    ECS::Global::get().registry->destroy(actor);
    std::cout << std::format("Actor destroyed with ID: {}\n", actorID);
}

void CabbageFramework::Actor::move(const std::array<float, 3> &pos)
{
    // 实现移动功能
}

void CabbageFramework::Actor::rotate(const std::array<float, 3> &euler)
{
    // 实现旋转功能
}

void CabbageFramework::Actor::scale(const std::array<float, 3> &size)
{
    // 实现缩放功能
}

void CabbageFramework::Actor::setWorldMatrix(const std::array<std::array<float, 4>, 4> &worldMartix)
{
    // 设置世界变换矩阵
}

std::array<std::array<float, 4>, 4> CabbageFramework::Actor::getWorldMatrix() const
{
    // 返回当前世界变换矩阵
    return std::array<std::array<float, 4>, 4>{};
}

void CabbageFramework::Actor::setMeshShape(const std::string &path)
{
    // 设置网格形状
}

void CabbageFramework::Actor::setSkeletalAnimation(const std::string &path)
{
    // 设置骨骼动画
}

uint64_t CabbageFramework::Actor::detectCollision(const Actor &other)
{
    // 碰撞检测实现
    return std::numeric_limits<uint64_t>::max();
}

void CabbageFramework::Actor::setOpticsParams(const OpticsParams &params)
{
    // 设置光学参数
}

void CabbageFramework::Actor::setAcousticsParams(const AcousticsParams &params)
{
    // 设置声学参数
}

void CabbageFramework::Actor::setMechanicsParams(const MechanicsParams &params)
{
    // 设置力学参数
}

