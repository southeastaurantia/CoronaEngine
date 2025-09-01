//
// Created by 47226 on 2025/8/22.
//

#include "AudioSystem.h"

#include <ECS/BackBridge.h>

#include <chrono>
#include <iostream>
#include <utility>

#define LOG_DEBUG(message)       \
    if constexpr (LOG_LEVEL < 1) \
    std::cout << std::format("[DEBUG][Audio] {}", message) << std::endl
#define LOG_INFO(message)        \
    if constexpr (LOG_LEVEL < 2) \
    std::cout << std::format("[INFO ][Audio] {}", message) << std::endl
#define LOG_WARNING(message)     \
    if constexpr (LOG_LEVEL < 3) \
    std::cout << std::format("[WARN ][Audio] {}", message) << std::endl
#define LOG_ERROR(message)       \
    if constexpr (LOG_LEVEL < 4) \
    std::cerr << std::format("[ERROR][Audio] {}", message) << std::endl

AudioSystem::AudioSystem(const std::shared_ptr<entt::registry> &registry)
    : running(false), registry(registry)
{
    // TODO: BackBridge事件注册

    LOG_INFO("Audio system initialized.");
}

void AudioSystem::stop()
{
    running.store(false);

    if (loopThread.joinable())
    {
        loopThread.join();
    }
    LOG_INFO("Audio system stopped");
}

AudioSystem::~AudioSystem()
{
    LOG_INFO("Animation system deconstruct.");
}

void AudioSystem::start()
{
    running.store(true);
    // 启动循环线程
    loopThread = std::thread(&AudioSystem::loop, this);
    LOG_INFO("Audio system started.");
}

void AudioSystem::loop()
{
    while (true)
    {
        if (!running.load())
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