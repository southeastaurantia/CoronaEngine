//
// Created by 47226 on 2025/9/5.
//

#include "RenderingSystemDefault.h"

#include "Core/Logger.h"

namespace Corona
{
    const char *RenderingSystemDefault::name()
    {
        return "RenderingSystemDefault";
    }
    RenderingSystemDefault::RenderingSystemDefault()
        : running(false)
    {
        Engine::inst().add_cmd_queue(name(), std::make_unique<SafeCommandQueue>());
    }
    RenderingSystemDefault::~RenderingSystemDefault()
    {
    }
    void RenderingSystemDefault::start()
    {
        constexpr int64_t TARGET_FRAME_TIME_US = 1000000 / 120;
        running.store(true);
        renderThread = std::thread([&] {
            while (running.load())
            {
                auto begin = std::chrono::high_resolution_clock::now();
                tick();
                auto end = std::chrono::high_resolution_clock::now();
                auto frameTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
                auto waitTimeUs = TARGET_FRAME_TIME_US - frameTimeUs;

                if (waitTimeUs > 0)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds(waitTimeUs));
                }
            }
        });
        LOG_DEBUG("{} started", name());
    }
    void RenderingSystemDefault::tick()
    {

    }
    void RenderingSystemDefault::stop()
    {
        running.store(false);
        if (renderThread.joinable())
        {
            renderThread.join();
        }
        LOG_DEBUG("{} stopped", name());
    }

    void RenderingSystemDefault::processRender(DataCache::id_type id)
    {
        updateEngine();
    }

    void RenderingSystemDefault::updateEngine()
    {
        // auto &sceneComponent = registry->get<ECS::Components::Scene>(scene);

        // gbufferPipeline(scene);
        // compositePipeline(sceneComponent);

        // sceneComponent.displayer = sceneComponent.finalOutputImage;
    }

    void RenderingSystemDefault::gbufferPipeline()
    {
        // auto &camera = registry->get<ECS::Components::Camera>(scene);
        // uniformBufferObjects.eyePosition = camera.pos;
        // uniformBufferObjects.eyeDir = ktm::normalize(camera.forward);
        //
        // uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(uniformBufferObjects.eyePosition, ktm::normalize(camera.forward), camera.worldUp);
        // uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(45.0f), (float)gbufferSize.x / (float)gbufferSize.y, 0.1f, 100.0f);
        //
        // gbufferUniformBufferObjects.viewProjMatrix = uniformBufferObjects.eyeProjMatrix * uniformBufferObjects.eyeViewMatrix;
        // gbufferUniformBuffer.copyFromData(&gbufferUniformBufferObjects, sizeof(gbufferUniformBufferObjects));
        //
        // gbufferPipelineObj["gbufferPosition"] = gbufferPostionImage;
        // gbufferPipelineObj["gbufferBaseColor"] = gbufferBaseColorImage;
        // gbufferPipelineObj["gbufferNormal"] = gbufferNormalImage;
        // gbufferPipelineObj["gbufferMotionVector"] = gbufferMotionVectorImage;
        // gbufferPipelineObj.executePipeline(gbufferSize);
    }

    void RenderingSystemDefault::compositePipeline()
    {
        // compositePipelineObj["pushConsts.gbufferSize"] = gbufferSize;
        // compositePipelineObj["pushConsts.gbufferPostionImage"] = gbufferPostionImage.storeDescriptor();
        // compositePipelineObj["pushConsts.gbufferBaseColorImage"] = gbufferBaseColorImage.storeDescriptor();
        // compositePipelineObj["pushConsts.gbufferNormalImage"] = gbufferNormalImage.storeDescriptor();
        // compositePipelineObj["pushConsts.gbufferDepthImage"] = gbufferPipelineObj.getDepthImage().storeDescriptor();
        //
        // compositePipelineObj["pushConsts.finalOutputImage"] = scene.finalOutputImage.storeDescriptor();
        //
        // compositePipelineObj["pushConsts.sun_dir"] = ktm::normalize(sunDir);
        //
        // compositePipelineObj["pushConsts.lightColor"] = ktm::fvec3(23.47f, 21.31f, 20.79f);
        //
        // uniformBuffer.copyFromData(&uniformBufferObjects, sizeof(uniformBufferObjects));
        // compositePipelineObj["pushConsts.uniformBufferIndex"] = uniformBuffer.storeDescriptor();

        // compositePipelineObj.executePipeline(ktm::uvec3(gbufferSize.x / 8, gbufferSize.y / 8, 1));
    }

} // namespace Corona