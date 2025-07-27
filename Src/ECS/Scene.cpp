#include "Scene.h"
#include "Events.hpp"

namespace ECS
{
    Scene::Scene() : animationSystem(std::make_unique<ECS::Systems::AnimationSystem>()),
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
        dispatcher.trigger<ECS::Events::SceneDestroy>();
        dispatcher.clear();
    }
} // namespace ECS