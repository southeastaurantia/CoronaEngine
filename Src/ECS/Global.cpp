#include "Global.h"

std::shared_ptr<entt::dispatcher> ECS::Global::Dispatcher = std::make_shared<entt::dispatcher>();
std::shared_ptr<entt::registry> ECS::Global::Registry = std::make_shared<entt::registry>();
std::shared_ptr<ECS::SceneManager> ECS::Global::SceneMgr = std::make_shared<ECS::SceneManager>();

namespace ECS
{

}