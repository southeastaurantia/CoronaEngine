#include "Core.h"

#include "BackBridge.h"
#include "Components.h"
#include "FrontBridge.h"

#include <format>

#define LOG_DEBUG(message)       \
    if constexpr (LOG_LEVEL < 1) \
    std::cout << std::format("[DEBUG][Core] {}", message) << std::endl
#define LOG_INFO(message)        \
    if constexpr (LOG_LEVEL < 2) \
    std::cout << std::format("[INFO ][Core] {}", message) << std::endl
#define LOG_WARNING(message)     \
    if constexpr (LOG_LEVEL < 3) \
    std::cout << std::format("[WARN ][Core] {}", message) << std::endl
#define LOG_ERROR(message)       \
    if constexpr (LOG_LEVEL < 4) \
    std::cout << std::format("[ERROR][Core] {}", message) << std::endl

namespace ECS
{
    Core::Core()
        : registry(std::make_shared<entt::registry>()),
          animation_system(registry),
          audio_system(registry),
          rendering_system(registry),
          resource_manager(registry)
    {
        // TODO: FrontBridge事件注册
        FrontBridge::dispatcher().sink<Events::SceneCreateRequest>().connect<&Core::onSceneCreate>(this);
        FrontBridge::dispatcher().sink<Events::SceneDestroy>().connect<&Core::onSceneDestroy>(this);
        FrontBridge::dispatcher().sink<Events::SceneSetDisplaySurface>().connect<&Core::onSceneSetDisplaySurface>(this);
        FrontBridge::dispatcher().sink<Events::SceneAddActor>().connect<&Core::onSceneAddActor>(this);
        FrontBridge::dispatcher().sink<Events::SceneRemoveActor>().connect<&Core::onSceneRemoveActor>(this);
        FrontBridge::dispatcher().sink<Events::SceneSetCamera>().connect<&Core::onSceneSetCamera>(this);

        FrontBridge::dispatcher().sink<Events::ActorCreateRequest>().connect<&Core::onActorCreate>(this);
        FrontBridge::dispatcher().sink<Events::ActorDestroy>().connect<&Core::onActorDestroy>(this);
        FrontBridge::dispatcher().sink<Events::ActorRotate>().connect<&Core::onActorRotate>(this);

        LOG_INFO("ECS::Core created");
    };

    Core::~Core()
    {
        LOG_INFO("ECS::Core destroyed");
    }

    void Core::onSceneCreate(Events::SceneCreateRequest event)
    {
        auto scene = registry->create();
        registry->emplace<Components::Scene>(scene, Components::Scene{});
        registry->emplace<Components::Camera>(scene, Components::Camera{});
        registry->emplace<Components::SunLight>(scene, Components::SunLight{});
        registry->emplace<Components::Actors>(scene, Components::Actors{});
        event.scene_id_promise->set_value(scene);
        LOG_INFO(std::format("Scene {} created", entt::to_entity(scene)));
    }

    void Core::onSceneDestroy(Events::SceneDestroy event)
    {
        for (const auto &actors = registry->get<Components::Actors>(event.scene).data;
             const auto &actor : actors)
        {
            auto &scenes = registry->get<Components::SceneRef>(actor).scenes;
            scenes.erase(event.scene);
            LOG_DEBUG(std::format("Before scene {} destroyed, remove reference from actor {}", entt::to_entity(event.scene), entt::to_entity(actor)));
        }

        registry->destroy(event.scene);
        LOG_INFO(std::format("Scene {} destroyed", entt::to_entity(event.scene)));
    }

    void Core::onSceneSetDisplaySurface(Events::SceneSetDisplaySurface event)
    {
        LOG_DEBUG(std::format("Scene {} published event 'SceneSetDisplaySurface'", entt::to_entity(event.scene)));
        auto &scene = registry->get<Components::Scene>(event.scene);
        scene.displaySurface = event.surface;
        BackBridge::render_dispatcher().enqueue(event);
    }

    void Core::onSceneAddActor(Events::SceneAddActor event)
    {
        auto &actors = registry->get<Components::Actors>(event.scene).data;
        if (std::ranges::find(actors, event.actor) != actors.end())
        {
            return;
        }
        actors.push_back(event.actor);

        auto &[scenes] = registry->get<Components::SceneRef>(event.actor);
        scenes.insert(event.scene);

        LOG_DEBUG(std::format("Scene {} added actor {}", entt::to_entity(event.scene), entt::to_entity(event.actor)));
    }

    void Core::onSceneRemoveActor(Events::SceneRemoveActor event)
    {
        auto &actors = registry->get<Components::Actors>(event.scene).data;

        if (std::ranges::find(actors, event.actor) == actors.end())
        {
            return;
        }

        std::erase(actors, event.actor);

        auto &scenes = registry->get<Components::SceneRef>(event.actor).scenes;
        scenes.erase(event.scene);

        LOG_DEBUG(std::format("Scene {} removed actor {}", entt::to_entity(event.scene), entt::to_entity(event.actor)));
    }

    void Core::onSceneSetCamera(Events::SceneSetCamera event)
    {
        auto &camera = registry->get<Components::Camera>(event.scene);
        camera.pos = {event.pos[0], event.pos[1], event.pos[2]};
        camera.forward = {event.forward[0], event.forward[1], event.forward[2]};
        camera.worldUp = {event.worldup[0], event.worldup[1], event.worldup[2]};
        camera.fov = event.fov;
        LOG_DEBUG(std::format("Scene {} processed event 'SceneSetCamera'", entt::to_entity(event.scene)));
    }

    void Core::onActorCreate(Events::ActorCreateRequest event)
    {
        auto actor = registry->create();
        registry->emplace<Components::ActorPose>(actor, Components::ActorPose{});
        registry->emplace<Components::Model>(actor, Components::Model{});
        registry->emplace<Components::SceneRef>(actor, Components::SceneRef{});
        event.actor_id_promise->set_value(actor);

        if(!event.path.empty())
        {
            entt::entity sharedModelEntity = entt::null;

            registry->view<Components::Meshes>().each([&](entt::entity modelEntity, Components::Meshes& meshes)
            {
                if (meshes.path == event.path)
                {
                    sharedModelEntity = modelEntity;
                }
            });

            if (sharedModelEntity != entt::null)
            {
                auto& modelComponent = registry->get<Components::Model>(actor);
                modelComponent.model = sharedModelEntity;
                LOG_INFO(std::format("Actor {} reused model from path: {}", entt::to_entity(actor), event.path));
            } else {
                auto modelEntity = registry->create();

                registry->emplace<Components::Animations>(modelEntity, Components::Animations{
                    .skeletalAnimations = {},
                    .boneInfoMap = {},
                    .boneCount = 0
                });

                resource_manager.LoadModel(modelEntity, event.path);

                auto& modelComponent = registry->get<Components::Model>(actor);
                modelComponent.model = modelEntity;
                LOG_INFO(std::format("Actor {} created new model from path: {}", entt::to_entity(actor), event.path));
            }
        }

        LOG_INFO(std::format("Actor {} created", entt::to_entity(actor)));
    }

    void Core::onActorDestroy(Events::ActorDestroy event)
    {
        for (const auto &scenes = registry->get<Components::SceneRef>(event.actor).scenes;
             const auto &scene : scenes)
        {
            auto &actors = registry->get<Components::Actors>(scene).data;
            std::erase(actors, event.actor);
            LOG_DEBUG(std::format("Before actor {} destroyed, remove reference from scene {}", entt::to_entity(event.actor), entt::to_entity(scene)));
        }

        registry->destroy(event.actor);
        LOG_INFO(std::format("Actor {} destroyed", entt::to_entity(event.actor)));
    }

    void Core::onActorRotate(Events::ActorRotate event)
    {
        auto &pose = registry->get<Components::ActorPose>(event.actor);
        pose.rotate = {
            event.euler[0],
            event.euler[1],
            event.euler[2]};
        LOG_DEBUG(std::format("Actor {} processed event 'ActorRotate'", entt::to_entity(event.actor)));
    }

} // namespace ECS