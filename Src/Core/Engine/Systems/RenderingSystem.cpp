#include "RenderingSystem.h"
#include "Core/Engine/Engine.h"
#include "Resource/Mesh.h"
#include "Resource/Model.h"

#include <memory>

using namespace Corona;

RenderingSystem::RenderingSystem()
    : ThreadedSystem("RenderingSystem")
{
    Engine::Instance().AddQueue(name(), std::make_unique<SafeCommandQueue>());
    init();
}

void RenderingSystem::onStart()
{
}

void RenderingSystem::onTick()
{
    // 最小消费命令队列，避免无限增长
    auto &rq = Engine::Instance().GetQueue(name());
    int spun = 0;
    while (spun < 100 && !rq.empty())
    {
        if (!rq.try_execute())
            continue;
        ++spun;
    }

    // 遍历 data_keys_ 示例：从 Cache<Mesh> 读取并执行占位渲染操作
    // auto &meshCache = Engine::Instance().Cache<Mesh>();
    // meshCache.safe_loop_foreach(data_keys_, [&](std::shared_ptr<Mesh> m) {
    //     (void)m; // TODO: 真正的渲染逻辑
    // });
    updateEngine();
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

void RenderingSystem::initShader(std::shared_ptr<Shader> shader)
{
    rasterizerPipeline = RasterizerPipeline(shader->vertCode, shader->fragCode);
    computePipeline = ComputePipeline(shader->computeCode);
    shaderHasInit = true;
}

void RenderingSystem::setDisplaySurface(std::shared_ptr<Scene> scene)
{
    scene->displayer.setSurface(scene->displaySurface);
    //scene->displayer = finalOutputImage;
}

void RenderingSystem::updateEngine()
{
    if (shaderHasInit)
    {
        auto &sceneCache = Engine::Instance().Cache<Scene>();
        sceneCache.safe_loop_foreach(scene_cache_keys_, [&](std::shared_ptr<Scene> scene) {
            if (!scene)
                return;

            gbufferPipeline(scene);
            compositePipeline();

            scene->displayer = finalOutputImage;
        });
    }
}
void RenderingSystem::gbufferPipeline(std::shared_ptr<Scene> scene)
{

    uniformBufferObjects.eyePosition = scene->camera.pos;
    uniformBufferObjects.eyeDir = ktm::normalize(scene->camera.forward);
    uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(uniformBufferObjects.eyePosition, ktm::normalize(scene->camera.forward), scene->camera.worldUp);
    uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(scene->camera.fov), (float)gbufferSize.x / (float)gbufferSize.y, 0.1f, 100.0f);

    gbufferUniformBufferObjects.viewProjMatrix = uniformBufferObjects.eyeProjMatrix * uniformBufferObjects.eyeViewMatrix;
    gbufferUniformBuffer.copyFromData(&gbufferUniformBufferObjects, sizeof(gbufferUniformBufferObjects));

    auto &modelCache = Engine::Instance().Cache<Model>();
    modelCache.safe_loop_foreach(model_cache_keys_, [&](std::shared_ptr<Model> model) {
        if (!model)
            return;

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
        
             rasterizerPipeline.startRecord(gbufferSize) << m.meshDevice->indexBuffer << rasterizerPipeline.endRecord();
         }
    });
}
void RenderingSystem::compositePipeline(ktm::fvec3 sunDir)
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

    computePipeline.executePipeline(ktm::uvec3(1920 / 8, 1080 / 8, 1));
}

void RenderingSystem::WatchModel(uint64_t id)
{
    model_cache_keys_.insert(id);
}

void RenderingSystem::UnwatchModel(uint64_t id)
{
    model_cache_keys_.erase(id);
}

void RenderingSystem::WatchScene(uint64_t id)
{
    scene_cache_keys_.insert(id);
}

void RenderingSystem::UnwatchScene(uint64_t id)
{
    scene_cache_keys_.erase(id);
}

void RenderingSystem::ClearWatched()
{
    scene_cache_keys_.clear();
    model_cache_keys_.clear();
}
