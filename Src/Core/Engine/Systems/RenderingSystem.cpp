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

    computeUniformBuffer = HardwareBuffer(sizeof(ComputeUniformBufferObject), BufferUsage::UniformBuffer);
    rasterizerUniformBuffer = HardwareBuffer(sizeof(RasterizerUniformBufferObject), BufferUsage::UniformBuffer);

    finalOutputImage = HardwareImage(gbufferSize, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
}

void RenderingSystem::initShader(std::shared_ptr<Shader> shader)
{
    rasterizerPipeline.initialize(shader->vertCode, shader->fragCode);
    computePipeline.initialize(shader->computeCode);
    shaderHasInit = true;
}

void RenderingSystem::setDisplaySurface(void *surface)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([surface]() mutable {
        auto &sys = Engine::Instance().GetSystem<RenderingSystem>();
        sys.displayers_.emplace_back(std::make_unique<HardwareDisplayer>(surface));
    });
}

void RenderingSystem::updateEngine()
{
    if (shaderHasInit)
    {
        for (auto &dptr : displayers_)
        {
            gbufferPipeline();
            compositePipeline();
            *dptr = finalOutputImage;
        }
    }
}
void RenderingSystem::gbufferPipeline()
{
    auto &modelCache = Engine::Instance().Cache<Model>();
    modelCache.safe_loop_foreach(data_keys_, [&](std::shared_ptr<Model> model) {
        if (!model)
            return;

    //     rasterizerUniformBuffer.copyFromData(&rasterizerUniformBufferObject, sizeof(rasterizerUniformBufferObject));
    //     rasterizerPipeline["pushConsts.modelMatrix"] = model->modelMatrix;
    //     rasterizerPipeline["pushConsts.uniformBufferIndex"] = rasterizerUniformBuffer.storeDescriptor();
    //     for (const auto &m : model->meshes)
    //     {
    //         rasterizerPipeline["inPosition"] = m.meshDevice->pointsBuffer;
    //         rasterizerPipeline["inNormal"] = m.meshDevice->normalsBuffer;
    //         rasterizerPipeline["inTexCoord"] = m.meshDevice->texCoordsBuffer;
    //         rasterizerPipeline["boneIndexes"] = m.meshDevice->boneIndexesBuffer;
    //         rasterizerPipeline["boneWeights"] = m.meshDevice->boneWeightsBuffer;
    //         rasterizerPipeline["pushConsts.textureIndex"] = m.meshDevice->textureIndex;
    //
    //         // rasterizerPipeline.
    //     }
    //
    // rasterizerPipeline.startRecord(gbufferSize) << rasterizerPipeline.endRecord();
    });
}
void RenderingSystem::compositePipeline()
{
    float time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - startTime).count();

    computeUniformData.imageID = finalOutputImage.storeDescriptor();
    computeUniformData.sunParams0.x = 0.0f;   // center X (compute shader flips Y)
    computeUniformData.sunParams0.y = 0.9f;   // near top in flipped space
    computeUniformData.sunParams0.z = 0.06f;  // smaller radius
    computeUniformData.sunParams0.w = time;   // pass time for cloud animation
    // sunset warm HDR color
    computeUniformData.sunColor = ktm::fvec4(12.0f, 6.0f, 2.0f, 0.0f);
    computeUniformBuffer.copyFromData(&computeUniformData, sizeof(computeUniformData));
    computePipeline["pushConsts.uniformBufferIndex"] = computeUniformBuffer.storeDescriptor();

    computePipeline.executePipeline(ktm::uvec3(1920 / 8, 1080 / 8, 1));
}

// static
void RenderingSystem::WatchMesh(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.insert(id);
    });
}

// static
void RenderingSystem::UnwatchMesh(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.erase(id);
    });
}

void RenderingSystem::WatchModel(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.insert(id);
    });
}

// static
void RenderingSystem::UnwatchModel(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.erase(id);
    });
}

// static
void RenderingSystem::ClearWatched()
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([&sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.clear();
    });
}
