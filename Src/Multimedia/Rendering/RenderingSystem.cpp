//
// Created by 47226 on 2025/8/22.
//

#include "RenderingSystem.h"

#include <ECS/BackBridge.h>

#include <chrono>
#include <utility>

RenderingSystem::RenderingSystem(std::shared_ptr<entt::registry> registry)
    : running(true), registry(std::move(registry))
{
    // TODO: BackBridge事件注册
    BackBridge::render_dispatcher().sink<ECS::Events::SceneSetDisplaySurface>().connect<&RenderingSystem::onSetDisplaySurface>(this);

    // 启动循环线程
    renderThread = std::thread(&RenderingSystem::renderLoop, this);
    displayThread = std::thread(&RenderingSystem::displayLoop, this);

    std::puts("Animation system started.");
}

RenderingSystem::~RenderingSystem()
{
    running = false;

    if (renderThread.joinable())
    {
        renderThread.join();
    }

    if (displayThread.joinable())
    {
        displayThread.join();
    }

    std::puts("Rendering system stoped.");
}

void RenderingSystem::renderLoop()
{
    while (true)
    {
        if (!running)
        {
            break;
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        BackBridge::render_dispatcher().update();
        /********** Do Something **********/

        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();

        if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < RenderMinFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((RenderMinFrameTime - frameTime) * 1000.0f)));
        }
    }
}

void RenderingSystem::displayLoop()
{
    while (true)
    {
        if (!running)
        {
            break;
        }

        auto startTime = std::chrono::high_resolution_clock::now();

        /********** Do Something **********/

        /********** Do Something **********/

        auto endTime = std::chrono::high_resolution_clock::now();

        if (const auto frameTime = std::chrono::duration<float>(endTime - startTime).count(); frameTime < DisplayMinFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>((DisplayMinFrameTime - frameTime) * 1000.0f)));
        }
    }
}

void RenderingSystem::onSetDisplaySurface(const ECS::Events::SceneSetDisplaySurface &event)
{
    std::puts(std::format("Scene {} set display surface.", entt::to_entity(event.scene)).c_str());
}