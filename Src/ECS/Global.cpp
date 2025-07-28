#include "Global.h"

std::unique_ptr<entt::dispatcher> ECS::Global::dispatcher = std::make_unique<entt::dispatcher>();
std::unique_ptr<entt::registry> ECS::Global::registry = std::make_unique<entt::registry>();

namespace ECS
{

}