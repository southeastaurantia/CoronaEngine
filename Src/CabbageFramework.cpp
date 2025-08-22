#include "CabbageFramework.h"

#include <ECS/Core.h>
#include <ECS/Events.hpp>
#include <ECS/FrontBridge.h>

static ECS::Core core;

// Actor实现
CabbageFramework::Actor::Actor(const std::string &path)
    : id(std::numeric_limits<uint64_t>::max()) // 创建Actor实体
{
    // 发送事件给ECS系统
}

CabbageFramework::Actor::~Actor()
{
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

uint64_t CabbageFramework::Actor::getID() const
{
    return id;
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

// Scene实现
CabbageFramework::Scene::Scene(void *surface, bool lightField)
    : id(std::numeric_limits<uint64_t>::max()) // 创建Scene实体
{
    FrontBridge::dispatcher().enqueue<ECS::Events::SceneCreate>({.surface = surface,
                                                                 .lightField = lightField});
    // TODO: 使用promise和future等待后端创建scene返回id
}

CabbageFramework::Scene::~Scene()
{
}

void CabbageFramework::Scene::setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov)
{
    // 设置相机参数
}

void CabbageFramework::Scene::setSunDirection(const std::array<float, 3> &direction)
{
    // 设置太阳光方向
}

void CabbageFramework::Scene::setDisplaySurface(void *surface)
{
}

CabbageFramework::Actor *CabbageFramework::Scene::detectActorByRay(const std::array<float, 3> &origin, const std::array<float, 3> &dir)
{
    return nullptr;
}

void CabbageFramework::Scene::addActor(const uint64_t &actor)
{
}

void CabbageFramework::Scene::removeActor(const uint64_t &actor)
{
}

uint64_t CabbageFramework::Scene::getID() const
{
    return id;
}