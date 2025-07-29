#include "Global.h"

ECS::Singleton &ECS::Singleton::get()
{
    static ECS::Singleton instance;
    return instance;
}

ECS::Singleton::Singleton() : dispatcher(std::make_shared<entt::dispatcher>()),
                              registry(std::make_shared<entt::registry>()),
                              sceneMgr(std::make_shared<ECS::SceneManager>()),
                              resourceMgr(std::make_shared<ECS::ResourceManager>()),
                              scheduler(std::make_shared<ECS::TaskScheduler>()),
                              mainloopThread(std::make_unique<std::thread>(&ECS::Singleton::mainloop, this))
{
}

ECS::Singleton::~Singleton()
{
    running = false;
    if (mainloopThread != nullptr)
    {
        mainloopThread->detach();
    }
}

void ECS::Singleton::mainloop()
{
    static constexpr float MaxFrameTime = 1.0f / 120.0f;

    while (running)
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        /********** Do Something **********/

        dispatcher->update();

        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration<float>(endTime - startTime).count();

        if (frameTime < MaxFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MaxFrameTime - frameTime) * 1000.0f)));
        }
    }
}
