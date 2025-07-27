#include "SceneManager.h"
#include "Global.h"

#include <iostream>

std::unordered_map<entt::entity, std::unique_ptr<ECS::Scene>> ECS::SceneManager::scenes = {};

namespace ECS
{

    entt::entity SceneManager::createScene()
    {
        return entt::null;
    }

    void SceneManager::removeScene(const entt::entity &scene)
    {
        if (scene == entt::null)
        {
            std::cout << "Cannot remove null scene\n";
            return;
        }
    }
} // namespace ECS