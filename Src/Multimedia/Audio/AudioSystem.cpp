// //
// // Created by 47226 on 2025/8/22.
// //
//
// #include "AudioSystem.h"
// #include "Utils/CabbageLogger.hpp"
//
// #include <ECS/BackBridge.h>
//
// #include <chrono>
//
// AudioSystem::AudioSystem(const std::shared_ptr<entt::registry> &registry)
//     : running(false), registry(registry)
// {
//     // TODO: BackBridge事件注册
//
//     LOG_INFO("Audio system initialized.");
// }
//
// void AudioSystem::stop()
// {
//     running.store(false);
//
//     if (loopThread.joinable())
//     {
//         loopThread.join();
//     }
//     LOG_INFO("Audio system stopped");
// }
//
// AudioSystem::~AudioSystem()
// {
//     LOG_INFO("Animation system deconstruct.");
// }
//
// void AudioSystem::start()
// {
//     running.store(true);
//     // 启动循环线程
//     loopThread = std::thread(&AudioSystem::loop, this);
//     LOG_INFO("Audio system started.");
// }
//
// void AudioSystem::loop()
// {
//     while (true)
//     {
//         if (!running.load())
//         {
//             break;
//         }
//
//         auto startTime = std::chrono::high_resolution_clock::now();
//         BackBridge::audio_dispatcher().update();
//         /********** Do Something **********/
//
//         /********** Do Something **********/
//
//         auto endTime = std::chrono::high_resolution_clock::now();
//
//         if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < MinFrameTime)
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((MinFrameTime - frameTime) * 1000.0f)));
//         }
//     }
// }