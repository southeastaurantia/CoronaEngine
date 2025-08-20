#include "Global.h"

#include <format>
#include <iostream>

ECS::Global &ECS::Global::get()
{
    static ECS::Global instance;
    return instance;
}

ECS::Global::Global() : dispatcher(std::make_shared<entt::dispatcher>()),
                        registry(std::make_shared<entt::registry>()),
                        resourceMgr(std::make_shared<ECS::ResourceManager>()),
                        scheduler(std::make_shared<ECS::TaskScheduler>()),
                        mainloopThread(std::make_unique<std::thread>(&ECS::Global::mainloop, this))
{
    this->dispatcher->sink<ECS::Events::CreateSceneEntity>().connect<&ECS::Global::onCreateSceneEntity>(this);
    this->dispatcher->sink<ECS::Events::DestroySceneEntity>().connect<&ECS::Global::onDestroySceneEntity>(this);
    this->dispatcher->sink<ECS::Events::CreateActorEntity>().connect<&ECS::Global::onCreatActorEntity>(this);
    this->dispatcher->sink<ECS::Events::DestroyActorEntity>().connect<&ECS::Global::onDestroyActorEntity>(this);
    this->dispatcher->sink<ECS::Events::SetDisplaySurface>().connect<&ECS::Global::onSetDisplaySurface>(this);

    std::cout << "ECS::Global created\n";
}

ECS::Global::~Global()
{
    running = false;
    if (mainloopThread != nullptr)
    {
        mainloopThread->join();
        std::cout << "Quited ECS::Global mainloop\n";
    }
    this->dispatcher->clear();
    std::cout << "ECS::Global destroyed\n";
}

void ECS::Global::mainloop()
{
    std::cout << "Start ECS::Global mainloop\n";
    static constexpr float MaxFrameTime = 1.0f / 120.0f;

    while (true)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        /********** Do Something **********/

        dispatcher->update();

        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<float>(endTime - startTime).count();

        if (running == false)
        {
            break;
        }

        if (frameTime < MaxFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MaxFrameTime - frameTime) * 1000.0f)));
        }
    }
}

void ECS::Global::onCreateSceneEntity(const ECS::Events::CreateSceneEntity &event)
{
    std::cout << std::format("Created SceneEntity : {}\n", static_cast<uint64_t>(event.scene));
    registry->emplace<ECS::Components::Camera>(event.scene, std::move(ECS::Components::Camera{}));
    registry->emplace<ECS::Components::SunLight>(event.scene, std::move(ECS::Components::SunLight{}));
    registry->emplace<ECS::Components::Actors>(event.scene, std::move(ECS::Components::Actors{.actors = {}}));

    auto scenePtr = std::make_shared<ECS::Global::Scene>();
    scenePtr->renderingSystem = std::make_shared<ECS::Systems::RenderingSystem>();
    scenePtr->audioSystem = std::make_shared<ECS::Systems::AudioSystem>();
    scenePtr->animationSystem = std::make_shared<ECS::Systems::AnimationSystem>();

    // System事件注册
    scenePtr->renderingSystem->dispatcher
        .sink<ECS::Events::EngineStart>()
        .connect<&ECS::ISystem::start>(scenePtr->renderingSystem.get());
    scenePtr->renderingSystem->dispatcher
        .sink<ECS::Events::EngineStop>()
        .connect<&ECS::ISystem::quit>(scenePtr->renderingSystem.get());
    scenePtr->renderingSystem->dispatcher
        .sink<ECS::Events::SetDisplaySurface>()
        .connect<&ECS::Systems::RenderingSystem::setDisplaySurface>(scenePtr->renderingSystem.get());

    scenePtr->audioSystem->dispatcher
        .sink<ECS::Events::EngineStart>()
        .connect<&ECS::ISystem::start>(scenePtr->audioSystem.get());
    scenePtr->audioSystem->dispatcher
        .sink<ECS::Events::EngineStop>()
        .connect<&ECS::ISystem::quit>(scenePtr->audioSystem.get());

    scenePtr->animationSystem->dispatcher
        .sink<ECS::Events::EngineStart>()
        .connect<&ECS::ISystem::start>(scenePtr->animationSystem.get());
    scenePtr->animationSystem->dispatcher
        .sink<ECS::Events::EngineStop>()
        .connect<&ECS::ISystem::quit>(scenePtr->animationSystem.get());

    scenePtr->renderingSystem->start();
    scenePtr->audioSystem->start();
    scenePtr->animationSystem->start();

    scenes[event.scene] = scenePtr;

    if (event.surface != nullptr)
    {
        scenePtr->renderingSystem->dispatcher.enqueue<ECS::Events::SetDisplaySurface>(ECS::Events::SetDisplaySurface{.surface = event.surface});
    }
}

void ECS::Global::onDestroySceneEntity(const ECS::Events::DestroySceneEntity &event)
{
    scenes[event.scene]->renderingSystem->quit();
    scenes[event.scene]->audioSystem->quit();
    scenes[event.scene]->animationSystem->quit();
    scenes.erase(event.scene);
    registry->destroy(event.scene);
    std::cout << std::format("Destroyed SceneEntity : {}\n", static_cast<uint64_t>(event.scene));
}

void ECS::Global::onCreatActorEntity(const ECS::Events::CreateActorEntity &event)
{
    std::cout << std::format("Created ActorEntity : {}\n", static_cast<uint64_t>(event.actor));
    registry->emplace<ECS::Components::ActorPose>(event.actor, std::move(ECS::Components::ActorPose{}));
    registry->emplace<ECS::Components::BoneMatrixDevice>(event.actor, std::move(ECS::Components::BoneMatrixDevice{}));
    registry->emplace<ECS::Components::BoneMatrixHost>(event.actor, std::move(ECS::Components::BoneMatrixHost{}));

    entt::entity modelEntity = registry->create();
    registry->emplace<ECS::Components::Model>(event.actor, std::move(ECS::Components::Model{.model = modelEntity}));

    // Actor事件注册

    auto &actorsCmpt = registry->get<ECS::Components::Actors>(event.scene);
    actorsCmpt.actors.push_back(event.actor);
    // TODO : 资源加载发送事件
    if(event.path != "")
    {
        ECS::Global::get().resourceMgr->loadDemo(event.path, modelEntity);
    }
}

void ECS::Global::onDestroyActorEntity(const ECS::Events::DestroyActorEntity &event)
{
    auto &actorsCmpt = registry->get<ECS::Components::Actors>(event.scene);
    actorsCmpt.actors.erase(std::remove(actorsCmpt.actors.begin(), actorsCmpt.actors.end(), event.actor), actorsCmpt.actors.end());
    registry->destroy(event.actor);

    std::cout << std::format("Destroyed ActorEntity : {}\n", static_cast<uint64_t>(event.actor));
}

void ECS::Global::onSetDisplaySurface(const ECS::Events::SetDisplaySurface &event)
{
    scenes[event.scene]->renderingSystem->dispatcher.enqueue<ECS::Events::SetDisplaySurface>(event);
}