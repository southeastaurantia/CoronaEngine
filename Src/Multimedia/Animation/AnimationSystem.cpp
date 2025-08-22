//
// Created by 47226 on 2025/8/22.
//

#include "AnimationSystem.h"

#include <ECS/BackBridge.h>

#include <chrono>
#include <utility>

AnimationSystem::AnimationSystem(std::shared_ptr<entt::registry> registry)
    : running(true), registry(std::move(registry))
{

    // TODO: BackBridge事件注册

    // 启动循环线程
    loopThread = std::thread(&AnimationSystem::loop, this);

    std::puts("Animation system started.");
}

AnimationSystem::~AnimationSystem()
{
    running = false;

    if (loopThread.joinable())
    {
        loopThread.join();
    }
    std::puts("Animation system stoped.");
}

void AnimationSystem::loop()
{
    while (true)
    {
        if (!running)
        {
            break;
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        BackBridge::anim_dispatcher().update();
        /********** Do Something **********/

        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();

        if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < MinFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MinFrameTime - frameTime) * 1000.0f)));
        }
    }
}