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
                        sceneMgr(std::make_shared<ECS::SceneManager>()),
                        resourceMgr(std::make_shared<ECS::ResourceManager>()),
                        scheduler(std::make_shared<ECS::TaskScheduler>()),
                        mainloopThread(std::make_unique<std::thread>(&ECS::Global::mainloop, this))
{
    this->dispatcher->sink<ECS::Events::CreateSceneEntity>().connect<&ECS::Global::onCreateSceneEntity>(this);
    this->dispatcher->sink<ECS::Events::DestroySceneEntity>().connect<&ECS::Global::onDestroySceneEntity>(this);

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
    this->sceneMgr->addScene(event.scene, std::make_shared<ECS::Scene>());
}

void ECS::Global::onDestroySceneEntity(const ECS::Events::DestroySceneEntity &event)
{
    this->sceneMgr->removeScene(event.scene);
}