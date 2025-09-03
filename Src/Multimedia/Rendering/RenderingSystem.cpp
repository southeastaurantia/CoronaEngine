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

static tbb::task_group tasks;

RenderingSystem::RenderingSystem(const std::shared_ptr<entt::registry> &registry)
    : running(false), registry(registry)
{
    // TODO: BackBridge事件注册
    BackBridge::render_dispatcher().sink<ECS::Events::SceneSetDisplaySurface>().connect<&RenderingSystem::onSetDisplaySurface>(this);

    LOG_INFO("Rendering system initialized.");
}

void RenderingSystem::stop()
{
    running.store(false);

    if (renderThread.joinable())
    {
        renderThread.join();
    }

    if (displayThread.joinable())
    {
        displayThread.join();
    }

    LOG_INFO("Rendering system stopped.");
}

RenderingSystem::~RenderingSystem()
{
    LOG_INFO("Rendering system deconstruct.");
}

void RenderingSystem::start()
{
    running.store(true);

    //gbufferPostionImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    //gbufferBaseColorImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    //gbufferNormalImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    //gbufferMotionVectorImage = HardwareImage(gbufferSize, ImageFormat::RG32_FLOAT, ImageUsage::StorageImage);

    //uniformBuffer = HardwareBuffer(sizeof(UniformBufferObject), BufferUsage::UniformBuffer);
    //gbufferUniformBuffer = HardwareBuffer(sizeof(gbufferUniformBufferObject), BufferUsage::UniformBuffer);

    // 启动循环线程
    renderThread = std::thread(&RenderingSystem::renderLoop, this);
    displayThread = std::thread(&RenderingSystem::displayLoop, this);

    LOG_INFO("Rendering system started.");
}

void RenderingSystem::renderLoop()
{
    while (true)
    {
        if (!running.load())
        {
            break;
        }

        auto startTime = std::chrono::high_resolution_clock::now();
        BackBridge::render_dispatcher().update();
        /********** Do Something **********/
        for (auto const &scenes = registry->view<ECS::Components::Scene>();
             auto const &scene : scenes)
        {
            tasks.run([this, scene] {
                updateEngine(scene);
            });
        }

        tasks.wait();
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
        if (!running.load())
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
    auto &sceneComponent = registry->get<ECS::Components::Scene>(scene);

    //gbufferPipeline(scene);
    //compositePipeline(sceneComponent);

    sceneComponent.displayer = sceneComponent.finalOutputImage;
}

//void RenderingSystem::gbufferPipeline(entt::entity scene)
//{
//   auto &camera = registry->get<ECS::Components::Camera>(scene);
//   uniformBufferObjects.eyePosition = camera.pos;
//   uniformBufferObjects.eyeDir = ktm::normalize(camera.forward);
//
//   uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(uniformBufferObjects.eyePosition, ktm::normalize(camera.forward), camera.worldUp);
//   uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(45.0f), (float)gbufferSize.x / (float)gbufferSize.y, 0.1f, 100.0f);
//
//   gbufferUniformBufferObjects.viewProjMatrix = uniformBufferObjects.eyeProjMatrix * uniformBufferObjects.eyeViewMatrix;
//   gbufferUniformBuffer.copyFromData(&gbufferUniformBufferObjects, sizeof(gbufferUniformBufferObjects));
//
//   gbufferPipelineObj["gbufferPosition"] = gbufferPostionImage;
//   gbufferPipelineObj["gbufferBaseColor"] = gbufferBaseColorImage;
//   gbufferPipelineObj["gbufferNormal"] = gbufferNormalImage;
//   gbufferPipelineObj["gbufferMotionVector"] = gbufferMotionVectorImage;
//   gbufferPipelineObj.executePipeline(gbufferSize);
//}


//void RenderingSystem::compositePipeline(ECS::Components::Scene scene,ktm::fvec3 sunDir) 
//{
//   compositePipelineObj["pushConsts.gbufferSize"] = gbufferSize;
//   compositePipelineObj["pushConsts.gbufferPostionImage"] = gbufferPostionImage.storeDescriptor();
//   compositePipelineObj["pushConsts.gbufferBaseColorImage"] = gbufferBaseColorImage.storeDescriptor();
//   compositePipelineObj["pushConsts.gbufferNormalImage"] = gbufferNormalImage.storeDescriptor();
//   compositePipelineObj["pushConsts.gbufferDepthImage"] = gbufferPipelineObj.getDepthImage().storeDescriptor();
//
//   compositePipelineObj["pushConsts.finalOutputImage"] = scene.finalOutputImage.storeDescriptor();
//
//   compositePipelineObj["pushConsts.sun_dir"] = ktm::normalize(sunDir);
//
//   compositePipelineObj["pushConsts.lightColor"] = ktm::fvec3(23.47f, 21.31f, 20.79f);
//
//   uniformBuffer.copyFromData(&uniformBufferObjects, sizeof(uniformBufferObjects));
//   compositePipelineObj["pushConsts.uniformBufferIndex"] = uniformBuffer.storeDescriptor();
//
//   compositePipelineObj.executePipeline(ktm::uvec3(gbufferSize.x / 8, gbufferSize.y / 8, 1));
//}
