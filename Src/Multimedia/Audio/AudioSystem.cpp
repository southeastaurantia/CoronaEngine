//
// Created by 47226 on 2025/8/22.
//

#include "AudioSystem.h"

#include <ECS/BackBridge.h>

#include <chrono>
#include <iostream>
#include <utility>

AudioSystem::AudioSystem(std::shared_ptr<entt::registry> registry)
    : running(true), registry(std::move(registry))
{
    // TODO: BackBridge事件注册

    // 启动循环线程
    loopThread = std::thread(&AudioSystem::loop, this);

    std::cout << "Audio system started." << std::endl;
}

AudioSystem::~AudioSystem()
{
    running = false;

    if (loopThread.joinable())
    {
        loopThread.join();
    }
    std::cout << "Audio system stoped." << std::endl;
}

void AudioSystem::loop()
{
    while (true)
    {
        if (!running)
        {
            break;
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        BackBridge::audio_dispatcher().update();
        /********** Do Something **********/

        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();

        if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < MinFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MinFrameTime - frameTime) * 1000.0f)));
        }
    }
}