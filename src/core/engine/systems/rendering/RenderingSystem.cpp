#include "RenderingSystem.h"

#include "core/engine/Engine.h"
#include "resource/Mesh.h"
#include "resource/Model.h"
#include "resource/Shader.h"

#include <memory>

using namespace Corona;

RenderingSystem::RenderingSystem()
    : ThreadedSystem("RenderingSystem")
{
    Engine::instance().add_queue(name(), std::make_unique<SafeCommandQueue>());
}

void RenderingSystem::onStart()
{
    init();
}

void RenderingSystem::onTick()
{
    // 最小消费命令队列，避免无限增长
    auto &rq = Engine::instance().get_queue(name());
    int spun = 0;
    while (spun < 100 && !rq.empty())
    {
        if (!rq.try_execute())
            continue;
        ++spun;
    }

    update_engine();
}

void RenderingSystem::onStop()
{
}

void RenderingSystem::init()
{
    gbufferPostionImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    gbufferBaseColorImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    gbufferNormalImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    gbufferMotionVectorImage = HardwareImage(gbufferSize, ImageFormat::RG32_FLOAT, ImageUsage::StorageImage);

    uniformBuffer = HardwareBuffer(sizeof(UniformBufferObject), BufferUsage::UniformBuffer);
    gbufferUniformBuffer = HardwareBuffer(sizeof(gbufferUniformBufferObject), BufferUsage::UniformBuffer);

    finalOutputImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
}

void RenderingSystem::init_shader(std::shared_ptr<Shader> shader)
{
    rasterizerPipeline = RasterizerPipeline(shader->vertCode, shader->fragCode);
    computePipeline = ComputePipeline(shader->computeCode);
    shaderHasInit = true;
}

void RenderingSystem::set_display_surface(std::shared_ptr<Scene> scene)
{
    scene->displayer.setSurface(scene->displaySurface);
    //scene->displayer = finalOutputImage;
}

void RenderingSystem::update_engine()
{
    if (shaderHasInit)
    {
    auto &scene_cache = Engine::instance().cache<Scene>();
    scene_cache.safe_loop_foreach(scene_cache_keys_, [&](std::shared_ptr<Scene> scene) {
            if (!scene)
                return;

            gbuffer_pipeline(scene);
            composite_pipeline();

            scene->displayer = finalOutputImage;
        });
    }
}
void RenderingSystem::gbuffer_pipeline(std::shared_ptr<Scene> scene)
{

    uniformBufferObjects.eyePosition = scene->camera.pos;
    uniformBufferObjects.eyeDir = ktm::normalize(scene->camera.forward);
    uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(uniformBufferObjects.eyePosition, ktm::normalize(scene->camera.forward), scene->camera.worldUp);
    uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(scene->camera.fov), (float)gbufferSize.x / (float)gbufferSize.y, 0.1f, 100.0f);

    gbufferUniformBufferObjects.viewProjMatrix = uniformBufferObjects.eyeProjMatrix * uniformBufferObjects.eyeViewMatrix;
    gbufferUniformBuffer.copyFromData(&gbufferUniformBufferObjects, sizeof(gbufferUniformBufferObjects));

    auto &model_cache = Engine::instance().cache<Model>();
    model_cache.safe_loop_foreach(model_cache_keys_, [&](std::shared_ptr<Model> model) {
        if (!model)
            return;

        model->getModelMatrix();
        ktm::fmat4x4 actorMatrix = model->modelMatrix;
        rasterizerPipeline["pushConsts.modelMatrix"] = actorMatrix;

        HardwareBuffer bonesMatrixBuffer = model->bonesMatrixBuffer;
        rasterizerPipeline["pushConsts.uniformBufferIndex"] = gbufferUniformBuffer.storeDescriptor();
        rasterizerPipeline["pushConsts.boneIndex"] = bonesMatrixBuffer.storeDescriptor();

        rasterizerPipeline["gbufferPostion"] = gbufferPostionImage;
        rasterizerPipeline["gbufferBaseColor"] = gbufferBaseColorImage;
        rasterizerPipeline["gbufferNormal"] = gbufferNormalImage;
        rasterizerPipeline["gbufferMotionVector"] = gbufferMotionVectorImage;

         for (auto &m : model->meshes)
         {
             rasterizerPipeline["inPosition"] = m.meshDevice->pointsBuffer;
             rasterizerPipeline["inNormal"] = m.meshDevice->normalsBuffer;
             rasterizerPipeline["inTexCoord"] = m.meshDevice->texCoordsBuffer;
             rasterizerPipeline["boneIndexes"] = m.meshDevice->boneIndexesBuffer;
             rasterizerPipeline["jointWeights"] = m.meshDevice->boneWeightsBuffer;
             rasterizerPipeline["pushConsts.textureIndex"] = m.meshDevice->textureIndex;
        
             executor(HardwareExecutor::ExecutorType::Graphics)
                 << rasterizerPipeline(gbufferSize.x, gbufferSize.y) << rasterizerPipeline.record(m.meshDevice->indexBuffer)
                 << executor.commit();
         }
    });
}
void RenderingSystem::composite_pipeline(ktm::fvec3 sunDir)
{

    computePipeline["pushConsts.gbufferSize"] = gbufferSize;
    computePipeline["pushConsts.gbufferPostionImage"] = gbufferPostionImage.storeDescriptor();
    computePipeline["pushConsts.gbufferBaseColorImage"] = gbufferBaseColorImage.storeDescriptor();
    computePipeline["pushConsts.gbufferNormalImage"] = gbufferNormalImage.storeDescriptor();
    computePipeline["pushConsts.gbufferDepthImage"] = rasterizerPipeline.getDepthImage().storeDescriptor();

    computePipeline["pushConsts.finalOutputImage"] = finalOutputImage.storeDescriptor();

    computePipeline["pushConsts.sun_dir"] = ktm::normalize(sunDir);

    computePipeline["pushConsts.lightColor"] = ktm::fvec3(23.47f, 21.31f, 20.79f);

    uniformBuffer.copyFromData(&uniformBufferObjects, sizeof(uniformBufferObjects));
    computePipeline["pushConsts.uniformBufferIndex"] = uniformBuffer.storeDescriptor();

    executor(HardwareExecutor::ExecutorType::Graphics)
        << computePipeline(1920 / 8, 1080 / 8, 1)
        << executor.commit();
}

void RenderingSystem::watch_model(uint64_t id)
{
    model_cache_keys_.insert(id);
}

void RenderingSystem::unwatch_model(uint64_t id)
{
    model_cache_keys_.erase(id);
}

void RenderingSystem::watch_scene(uint64_t id)
{
    scene_cache_keys_.insert(id);
}

void RenderingSystem::unwatch_scene(uint64_t id)
{
    scene_cache_keys_.erase(id);
}

void RenderingSystem::clear_watched()
{
    scene_cache_keys_.clear();
    model_cache_keys_.clear();
}
