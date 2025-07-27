#include "Scene.h"
#include "Events.hpp"

#include <Multimedia/Animation/AnimationSystem.h>
#include <Multimedia/Audio/AudioSystem.h>
#include <Multimedia/Rendering/RenderingSystem.h>

namespace ECS
{
    Scene::Scene() : dispatcher(entt::dispatcher{}),
                     animationSystem(std::make_unique<ECS::Systems::AnimationSystem>()),
                     audioSystem(std::make_unique<ECS::Systems::AudioSystem>()),
                     renderingSystem(std::make_unique<ECS::Systems::RenderingSystem>())
    {
        animationSystem->registerEvents(dispatcher);
        audioSystem->registerEvents(dispatcher);
        renderingSystem->registerEvents(dispatcher);

        dispatcher.trigger<ECS::Events::SceneCreate>();
    }

    Scene::~Scene()
    {
        dispatcher.trigger<ECS::Events::SceneRemove>();
        dispatcher.clear();
    }
} // namespace ECS