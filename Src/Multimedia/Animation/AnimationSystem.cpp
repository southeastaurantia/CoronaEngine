//
// Created by 47226 on 2025/8/22.
//

#include "AnimationSystem.h"
#include "Utils/CabbageLogger.hpp"

#include <ECS/BackBridge.h>

#include <chrono>
#include <iostream>
#include <utility>

AnimationSystem::AnimationSystem(const std::shared_ptr<entt::registry> &registry)
    : running(false), registry(registry)
{

    // TODO: BackBridge事件注册

    LOG_INFO("Animation system initialized.");
}

void AnimationSystem::stop()
{
    running.store(false);

    if (loopThread.joinable())
    {
        loopThread.join();
    }
    LOG_INFO("Animation system stopped.");
}

AnimationSystem::~AnimationSystem()
{
    LOG_INFO("Animation system deconstruct.");
}

void AnimationSystem::start()
{
    running.store(true);
    // 启动循环线程
    loopThread = std::thread(&AnimationSystem::loop, this);
    LOG_INFO("Animation system started.");
}

void AnimationSystem::loop()
{
    while (true)
    {
        if (!running.load())
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