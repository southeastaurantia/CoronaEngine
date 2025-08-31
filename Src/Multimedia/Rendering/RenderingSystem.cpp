//
// Created by 47226 on 2025/8/22.
//

#include "RenderingSystem.h"
#include "entt/entity/entity.hpp"

#include <ECS/BackBridge.h>
#include <ECS/Core.h>

#include <chrono>
#include <iostream>
#include <utility>

#define LOG_DEBUG(message)       \
    if constexpr (LOG_LEVEL < 1) \
    std::cout << std::format("[DEBUG][Render] {}", message) << std::endl
#define LOG_INFO(message)        \
    if constexpr (LOG_LEVEL < 2) \
    std::cout << std::format("[INFO ][Render] {}", message) << std::endl
#define LOG_WARNING(message)     \
    if constexpr (LOG_LEVEL < 3) \
    std::cout << std::format("[WARN ][Render] {}", message) << std::endl
#define LOG_ERROR(message)       \
    if constexpr (LOG_LEVEL < 4) \
    std::cerr << std::format("[ERROR][Render] {}", message) << std::endl

RenderingSystem::RenderingSystem(std::shared_ptr<entt::registry> registry)
    : running(true), registry(std::move(registry))
{
    // TODO: BackBridge事件注册
    BackBridge::render_dispatcher().sink<ECS::Events::SceneSetDisplaySurface>().connect<&RenderingSystem::onSetDisplaySurface>(this);

    // 启动循环线程
    renderThread = std::thread(&RenderingSystem::renderLoop, this);
    displayThread = std::thread(&RenderingSystem::displayLoop, this);

    LOG_INFO("Rendering system initialized & started.");
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

    LOG_INFO("Rendering system stopped & destroyed.");
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
        auto const &scenes = registry->view<ECS::Components::Scene>();
        for (auto scene : scenes)
        {
            updateEngine(scene);
        }
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

void RenderingSystem::onSetDisplaySurface(const ECS::Events::SceneSetDisplaySurface event)
{
    auto &scene = registry->get<ECS::Components::Scene>(event.scene);
    scene.displayer = HardwareDisplayer(event.surface);
    scene.finalOutputImage = HardwareImage(ktm::uvec2(800, 800), ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    scene.displayer = scene.finalOutputImage;
    LOG_INFO(std::format("Scene {} set display surface.", entt::to_entity(event.scene)));
}

void RenderingSystem::updateEngine(entt::entity scene)
{
    auto& sceneComponent = registry->get<ECS::Components::Scene>(scene);
    sceneComponent.displayer = sceneComponent.finalOutputImage;
}