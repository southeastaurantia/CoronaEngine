#include "RenderingSystem.h"

#include "Core/Engine/Engine.h"
#include "Core/Log.h"
#include "Resource/Mesh.h"
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
    while (spun < 16 && rq.try_execute())
        ++spun;

    // 遍历 data_keys_ 示例：从 Cache<Mesh> 读取并执行占位渲染操作
    auto &meshCache = Engine::Instance().Cache<Mesh>();
    meshCache.safe_loop_foreach(data_keys_, [&](std::shared_ptr<Mesh> m) {
        (void)m; // TODO: 真正的渲染逻辑
    });
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
}

void RenderingSystem::setDisplaySurface(void *surface)
{
    HardwareDisplayer displayer(surface);
    HardwareImage finalOutputImage(ktm::uvec2(1920, 1080), ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    displayer = finalOutputImage;
}

void RenderingSystem::updateEngine()
{
    // 按旧逻辑分阶段：GBuffer -> Composite
    gbufferPipeline();
    compositePipeline();
}
void RenderingSystem::gbufferPipeline()
{
    // uniformBufferObjects.eyePosition = scene->getCamera().pos;
    // uniformBufferObjects.eyeDir = ktm::normalize(scene->getCamera().forward);
    //
    // uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(uniformBufferObjects.eyePosition, ktm::normalize(camera.forward), camera.worldUp);
    // uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(45.0f), (float)gbufferSize.x / (float)gbufferSize.y, 0.1f, 100.0f);
    //
    // gbufferUniformBufferObjects.viewProjMatrix = uniformBufferObjects.eyeProjMatrix * uniformBufferObjects.eyeViewMatrix;
    // gbufferUniformBuffer.copyFromData(&gbufferUniformBufferObjects, sizeof(gbufferUniformBufferObjects));
    //
    // std::set<CabbageEngine::Actor *> actors = scene->getActors();
    // for (auto &actor : actors)
    // {
    //     ktm::fmat4x4 actorMatrix = actor->getActorMatrix();
    //     gbufferPipleLine["pushConsts.modelMatrix"] = actorMatrix;
    //
    //     HardwareBuffer bonesMatrixBuffer = actor->getBonesMatrixBuffer();
    //     gbufferPipleLine["pushConsts.uniformBufferIndex"] = gbufferUniformBuffer.storeDescriptor();
    //     gbufferPipleLine["pushConsts.boneIndex"] = bonesMatrixBuffer.storeDescriptor();
    //
    //     std::vector<CabbageEngine::Actor::MeshDeviceData> deviceMeshes = actor->getDeviceMeshes();
    //     for (auto &geom : deviceMeshes)
    //     {
    //         gbufferPipleLine["inPosition"] = geom.pointsBuffer;
    //         gbufferPipleLine["inNormal"] = geom.normalsBuffer;
    //         gbufferPipleLine["inTexCoord"] = geom.texCoordsBuffer;
    //         gbufferPipleLine["boneIndexes"] = geom.boneIndexesBuffer;
    //         gbufferPipleLine["jointWeights"] = geom.boneWeightsBuffer;
    //         gbufferPipleLine["pushConsts.textureIndex"] = geom.textureIndex;
    //
    //         gbufferPipleLine.recordGeomMesh(geom.indexBuffer);
    //     }
    // }
    //
    // gbufferPipleLine["gbufferPostion"] = gbufferPostionImage;
    // gbufferPipleLine["gbufferBaseColor"] = gbufferBaseColorImage;
    // gbufferPipleLine["gbufferNormal"] = gbufferNormalImage;
    // gbufferPipleLine["gbufferMotionVector"] = gbufferMotionVectorImage;
    // gbufferPipleLine.executePipeline(gbufferSize);
    CE_LOG_DEBUG("RenderingSystem: gbufferPipeline executed");
}
void RenderingSystem::compositePipeline()
{
    // rendererPipleLine["pushConsts.gbufferSize"] = gbufferSize;
    // rendererPipleLine["pushConsts.gbufferPostionImage"] = gbufferPostionImage.storeDescriptor();
    // rendererPipleLine["pushConsts.gbufferBaseColorImage"] = gbufferBaseColorImage.storeDescriptor();
    // rendererPipleLine["pushConsts.gbufferNormalImage"] = gbufferNormalImage.storeDescriptor();
    // rendererPipleLine["pushConsts.gbufferDepthImage"] = gbufferPipleLine.getDepthImage().storeDescriptor();
    //
    // rendererPipleLine["pushConsts.finalOutputImage"] = finalOutputImage.storeDescriptor();
    //
    // rendererPipleLine["pushConsts.sun_dir"] = ktm::normalize(sunDir);
    //
    // rendererPipleLine["pushConsts.lightColor"] = ktm::fvec3(23.47f, 21.31f, 20.79f);
    //
    // uniformBuffer.copyFromData(&uniformBufferObjects, sizeof(uniformBufferObjects));
    // rendererPipleLine["pushConsts.uniformBufferIndex"] = uniformBuffer.storeDescriptor();
    //
    // rendererPipleLine.executePipeline(ktm::uvec3(gbufferSize.x / 8, gbufferSize.y / 8, 1));
    CE_LOG_DEBUG("RenderingSystem: compositePipeline executed");
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

// static
void RenderingSystem::ClearWatched()
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([&sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.clear();
    });
}
