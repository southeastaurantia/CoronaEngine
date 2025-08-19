#pragma once

#include "TaskScheduler.h"

#include <ECS/Events.hpp>
#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>
#include <Resource/ResourceManager.h>

#include <entt/entt.hpp>

namespace ECS
{
    class Global final
    {
        struct Scene final
        {
            std::shared_ptr<ECS::Systems::AnimationSystem> animationSystem;
            std::shared_ptr<ECS::Systems::AudioSystem> audioSystem;
            std::shared_ptr<ECS::Systems::RenderingSystem> renderingSystem;
            std::unordered_set<entt::entity> actors;
        };

      public:
        static Global &get();

        std::shared_ptr<entt::dispatcher> dispatcher;
        std::shared_ptr<entt::registry> registry;
        std::shared_ptr<ECS::ResourceManager> resourceMgr;
        std::shared_ptr<ECS::TaskScheduler> scheduler;

      private:
        Global();
        ~Global();

        void mainloop();

        bool running{true};
        std::unique_ptr<std::thread> mainloopThread;
        std::unordered_map<entt::entity, std::shared_ptr<ECS::Global::Scene>> scenes{};

        void onCreateSceneEntity(const ECS::Events::CreateSceneEntity &event);
        void onDestroySceneEntity(const ECS::Events::DestroySceneEntity &event);
        void onCreatActorEntity(const ECS::Events::CreateActorEntity &event);
        void onDestroyActorEntity(const ECS::Events::DestroyActorEntity &event);
        void onSetDisplaySurface(const ECS::Events::SetDisplaySurface &event);
    };
} // namespace ECS