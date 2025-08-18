#pragma once

#include "ISystem.h"

#include <unordered_map>

namespace ECS
{
    class Scene final
    {
      public:
        Scene(entt::entity sceneId, void *surface, bool lightField);
        ~Scene();

        void setCamera(const std::array<float, 3> &pos, const std::array<float, 3> &forward, const std::array<float, 3> &worldup, const float &fov);
        void setSunDirection(const std::array<float, 3> &direction);
        void setDisplaySurface(void *surface);

      private:
        entt::dispatcher dispatcher;
        std::shared_ptr<ECS::ISystem> animationSystem;
        std::shared_ptr<ECS::ISystem> audioSystem;
        std::shared_ptr<ECS::ISystem> renderingSystem;
        entt::entity sceneId;
    };

    class SceneManager final
    {
      public:
        SceneManager();
        ~SceneManager();

        void removeScene(entt::entity id);
        void addScene(entt::entity id, std::shared_ptr<ECS::Scene> scene);
        entt::entity createScene(void *surface, bool lightField);
        std::shared_ptr<ECS::Scene> getScene(entt::entity id) const;

      private:
        std::unordered_map<entt::entity, std::shared_ptr<ECS::Scene>> scenes;
    };

} // namespace ECS