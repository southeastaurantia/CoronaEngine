// #include "CabbageFramework.h"
//
// #include <ECS/Core.h>
// #include <ECS/Events.hpp>
// #include <ECS/FrontBridge.h>
// #include <Resource/ResourceManager.h>
// #include <filesystem>
// #include <format>
//
// // static ECS::Core core{};
// // std::shared_ptr<entt::registry> ECS::ResourceManager::registry = core.getRegistry();
// // std::string ECS::ResourceManager::basePath;
// // std::string ECS::ResourceManager::userPath;
//
// struct Corona::ActorImpl final
// {
//     friend struct Actor;
//     friend struct SceneImpl;
//
//   private:
//     ActorImpl(const std::string &path = "")
//         : id(entt::null)
//     {
//         const auto id_promise = std::make_shared<std::promise<entt::entity>>();
//         std::future<entt::entity> id_future = id_promise->get_future();
//
//         FrontBridge::dispatcher().trigger(Corona::Events::ActorCreateRequest{.path = path, .actor_id_promise = id_promise});
//         id = id_future.get();
//     }
//
//     ~ActorImpl()
//     {
//         FrontBridge::dispatcher().trigger(Corona::Events::ActorDestroy{.actor = id});
//     }
//
//     void move(const std::array<float, 3> &pos)
//     {
//         // 实现移动功能
//     }
//
//     void rotate(const std::array<float, 3> &euler)
//     {
//         FrontBridge::dispatcher().trigger(Corona::Events::ActorRotate{.actor = id, .euler = euler});
//     }
//
//     void scale(const std::array<float, 3> &size)
//     {
//         // 实现缩放功能
//     }
//
//     void setWorldMatrix(const std::array<std::array<float, 4>, 4> &worldMartix)
//     {
//         // 设置世界变换矩阵
//     }
//
//     std::array<std::array<float, 4>, 4> getWorldMatrix() const
//     {
//         // 返回当前世界变换矩阵
//         return std::array<std::array<float, 4>, 4>{};
//     }
//
//     void setMeshShape(const std::string &path)
//     {
//         // 设置网格形状
//     }
//
//     void setSkeletalAnimation(const std::string &path)
//     {
//         // 设置骨骼动画
//     }
//
//     uint64_t detectCollision(const ActorImpl &other)
//     {
//         // 碰撞检测实现
//         return std::numeric_limits<uint64_t>::max();
//     }
//
//     [[nodiscard]] uint64_t getID() const
//     {
//         return entt::to_entity(id);
//     }
//
//   private:
//     entt::entity id;
// };
//
// struct Corona::SceneImpl final
// {
//     friend struct Scene;
//
//   private:
//     explicit SceneImpl(void *surface = nullptr, const bool lightField = false)
//         : id(entt::null)
//     {
//         const auto id_promise = std::make_shared<std::promise<entt::entity>>();
//         std::future<entt::entity> id_future = id_promise->get_future();
//
//         FrontBridge::dispatcher().trigger(Corona::Events::SceneCreateRequest{.surface = surface, .lightField = lightField, .scene_id_promise = id_promise});
//         id = id_future.get();
//
//         if (surface)
//         {
//             setDisplaySurface(surface);
//         }
//     }
//
//     ~SceneImpl()
//     {
//         FrontBridge::dispatcher().trigger(Corona::Events::SceneDestroy{.scene = id});
//     }
//
//     void setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov)
//     {
//         // 设置相机参数
//         FrontBridge::dispatcher().trigger(Corona::Events::SceneSetCamera{.scene = id, .pos = pos, .forward = forward, .worldup = worldup, .fov = fov});
//     }
//
//     void setSunDirection(const std::array<float, 3> &direction)
//     {
//         // 设置太阳光方向
//     }
//
//     void setDisplaySurface(void *surface)
//     {
//         FrontBridge::dispatcher().trigger(Corona::Events::SceneSetDisplaySurface{.scene = id, .surface = surface});
//     }
//
//     entt::entity detectActorByRay(const std::array<float, 3> &origin, const std::array<float, 3> &dir)
//     {
//         return entt::null;
//     }
//
//     void addActor(const ActorImpl *actor)
//     {
//         FrontBridge::dispatcher().trigger(Corona::Events::SceneAddActor{.scene = id, .actor = actor->id});
//     }
//
//     void removeActor(const ActorImpl *actor)
//     {
//         FrontBridge::dispatcher().trigger(Corona::Events::SceneRemoveActor{.scene = id, .actor = actor->id});
//     }
//
//     [[nodiscard]] uint64_t getID() const
//     {
//         return entt::to_entity(id);
//     }
//
//   private:
//     entt::entity id;
// };
//
// /************************************ 以下是API ********************************************/
//
// Corona::Actor::Actor(const std::string &path)
//     : impl(new ActorImpl(path)),
//       ref_count(new int(1))
// {
// }
//
// Corona::Actor::Actor(const Actor &other)
//     : impl(other.impl),
//       ref_count(other.ref_count)
// {
//     if (ref_count)
//     {
//         ++(*ref_count);
//     }
// }
//
// Corona::Actor::Actor(Actor &&other) noexcept
//     : impl(other.impl),
//       ref_count(other.ref_count)
// {
//     other.impl = nullptr;
//     other.ref_count = nullptr;
// }
//
// Corona::Actor::~Actor()
// {
//     if (ref_count && (--(*ref_count) == 0))
//     {
//         delete impl;
//         delete ref_count;
//         impl = nullptr;
//         ref_count = nullptr;
//     }
// }
//
// Corona::Actor &Corona::Actor::operator=(const Actor &other)
// {
//     if (this != &other)
//     {
//         if (ref_count && (--(*ref_count) == 0))
//         {
//             delete impl;
//             delete ref_count;
//         }
//
//         impl = other.impl;
//         ref_count = other.ref_count;
//
//         if (ref_count)
//         {
//             ++(*ref_count);
//         }
//     }
//     return *this;
// }
//
// Corona::Actor &Corona::Actor::operator=(Actor &&other) noexcept
// {
//     if (this != &other)
//     {
//         if (ref_count && (--(*ref_count) == 0))
//         {
//             delete impl;
//             delete ref_count;
//         }
//
//         impl = other.impl;
//         ref_count = other.ref_count;
//
//         other.impl = nullptr;
//         other.ref_count = nullptr;
//     }
//     return *this;
// }
//
// void Corona::Actor::move(const std::array<float, 3> &pos) const
// {
//     impl->move(pos);
// }
//
// void Corona::Actor::rotate(const std::array<float, 3> &euler) const
// {
//     impl->rotate(euler);
// }
//
// void Corona::Actor::scale(const std::array<float, 3> &size) const
// {
//     impl->scale(size);
// }
//
// void Corona::Actor::setWorldMatrix(const std::array<std::array<float, 4>, 4> &worldMartix) const
// {
//     impl->setWorldMatrix(worldMartix);
// }
//
// std::array<std::array<float, 4>, 4> Corona::Actor::getWorldMatrix() const
// {
//     return impl->getWorldMatrix();
// }
//
// void Corona::Actor::setMeshShape(const std::string &path) const
// {
//     impl->setMeshShape(path);
// }
//
// void Corona::Actor::setSkeletalAnimation(const std::string &path) const
// {
//     impl->setSkeletalAnimation(path);
// }
//
// uint64_t Corona::Actor::detectCollision(const ActorImpl &other)
// {
//     return impl->detectCollision(other);
// }
//
// uint64_t Corona::Actor::getID() const
// {
//     return impl->getID();
// }
//
// Corona::ActorImpl *Corona::Actor::get() const
// {
//     return impl;
// }
//
// Corona::Scene::Scene(void *surface, const bool lightField)
//     : impl(new SceneImpl(surface, lightField)),
//       ref_count(new int(1))
// {
// }
//
// Corona::Scene::Scene(const Scene &other)
//     : impl(other.impl),
//       ref_count(other.ref_count)
// {
//     if (ref_count)
//     {
//         ++(*ref_count);
//     }
// }
//
// Corona::Scene::Scene(Scene &&other) noexcept
//     : impl(other.impl),
//       ref_count(other.ref_count)
// {
//     other.impl = nullptr;
//     other.ref_count = nullptr;
// }
//
// Corona::Scene::~Scene()
// {
//     if (ref_count && (--(*ref_count) == 0))
//     {
//         delete impl;
//         delete ref_count;
//         impl = nullptr;
//         ref_count = nullptr;
//     }
// }
//
// Corona::Scene &Corona::Scene::operator=(const Scene &other)
// {
//     if (this != &other)
//     {
//         if (ref_count && (--(*ref_count) == 0))
//         {
//             delete impl;
//             delete ref_count;
//         }
//
//         impl = other.impl;
//         ref_count = other.ref_count;
//
//         if (ref_count)
//         {
//             ++(*ref_count);
//         }
//     }
//     return *this;
// }
//
// Corona::Scene &Corona::Scene::operator=(Scene &&other) noexcept
// {
//     if (this != &other)
//     {
//         if (ref_count && (--(*ref_count) == 0))
//         {
//             delete impl;
//             delete ref_count;
//         }
//
//         impl = other.impl;
//         ref_count = other.ref_count;
//
//         other.impl = nullptr;
//         other.ref_count = nullptr;
//     }
//     return *this;
// }
//
// void Corona::Scene::setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov) const
// {
//     impl->setCamera(pos, forward, worldup, fov);
// }
//
// void Corona::Scene::setSunDirection(const std::array<float, 3> &direction) const
// {
//     impl->setSunDirection(direction);
// }
//
// void Corona::Scene::setDisplaySurface(void *surface) const
// {
//     impl->setDisplaySurface(surface);
// }
//
// uint64_t Corona::Scene::detectActorByRay(const std::array<float, 3> &origin, const std::array<float, 3> &dir) const
// {
//     return entt::to_entity(impl->detectActorByRay(origin, dir));
// }
//
// void Corona::Scene::addActor(const Actor &actor) const
// {
//     impl->addActor(actor.get());
// }
//
// void Corona::Scene::removeActor(const Actor &actor) const
// {
//     impl->removeActor(actor.get());
// }
//
// uint64_t Corona::Scene::getID() const
// {
//     return impl->getID();
// }
//
// Corona::SceneImpl *Corona::Scene::get() const
// {
//     return impl;
// }