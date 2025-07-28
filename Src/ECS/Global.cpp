#include "Global.h"

std::shared_ptr<entt::dispatcher> ECS::Global::Dispatcher = std::make_shared<entt::dispatcher>();
std::shared_ptr<entt::registry> ECS::Global::Registry = std::make_shared<entt::registry>();
std::shared_ptr<ECS::SceneManager> ECS::Global::SceneMgr = std::make_shared<ECS::SceneManager>();
std::shared_ptr<ECS::ResourceManager> ECS::Global::ResourceMgr = std::make_shared<ECS::ResourceManager>();
std::shared_ptr<ECS::TaskScheduler> ECS::Global::TaskScheduler = std::make_shared<ECS::TaskScheduler>();

namespace ECS
{

}