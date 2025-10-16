#include "RenderingSystem.h"

#include "Engine.h"
#include "Mesh.h"
#include "Model.h"
#include "Shader.h"

#include <memory>
#include <filesystem>

using namespace Corona;

RenderingSystem::RenderingSystem()
    : ThreadedSystem("RenderingSystem")
{
    Engine::instance().add_queue(name(), std::make_unique<SafeCommandQueue>());
}

void RenderingSystem::onStart()
{
    init();

    // 订阅场景事件（摄像机/太阳光/显示表面/移除）
    cam_sub_ = Engine::instance().events<SceneEvents::CameraUpdated>().subscribe("scene.camera");
    sun_sub_ = Engine::instance().events<SceneEvents::SunUpdated>().subscribe("scene.sun");
    surf_sub_ = Engine::instance().events<SceneEvents::DisplaySurfaceUpdated>().subscribe("scene.surface");
    rm_sub_ = Engine::instance().events<SceneEvents::Removed>().subscribe("scene.removed");

    // 订阅 Actor 事件（纯 ECS + 事件流）
    actor_spawn_sub_ = Engine::instance().events<ActorEvents::Spawned>().subscribe("actor.spawn");
    actor_tfm_sub_   = Engine::instance().events<ActorEvents::TransformUpdated>().subscribe("actor.transform");
    actor_rm_sub_    = Engine::instance().events<ActorEvents::Removed>().subscribe("actor.removed");

    // 测试样例：系统内部使用资源管理器异步加载 shader，并把结果回投到本系统队列
    const auto assets_root = (std::filesystem::current_path() / "assets").string();
    auto shaderId = ResourceId::from("shader", assets_root);
    auto sys_name = std::string{name()};
    Engine::instance().resources().load_once_async(
        shaderId,
        [sys_name](const ResourceId&, std::shared_ptr<IResource> r) {
            auto &engine = Engine::instance();
            auto &q = engine.get_queue(sys_name);
            if (!r) {
                q.enqueue([] { CE_LOG_WARN("[RenderingSystem] 异步加载 shader 失败"); });
                return;
            }
            q.enqueue([] { CE_LOG_INFO("[RenderingSystem] 异步加载 shader 成功（测试样例）"); });
        });
}

void RenderingSystem::onTick()
{
    // 1) 消费渲染系统命令队列，避免堆积
    auto &rq = Engine::instance().get_queue(name());
    int spun = 0;
    while (spun < 128 && !rq.empty())
    {
        if (!rq.try_execute())
            continue;
        ++spun;
    }

    // 2) 拉取场景事件，更新本地快照（每帧有限次，避免消息风暴）
    {
        SceneEvents::CameraUpdated ev{}; int handled = 0;
        while (handled < 64 && cam_sub_.queue && cam_sub_.queue->try_pop(ev)) {
            auto &snap = scene_snapshots_[ev.sceneId];
            snap.camera.fov = ev.fov;
            snap.camera.pos = ev.pos;
            snap.camera.forward = ev.forward;
            snap.camera.worldUp = ev.worldUp;
            ++handled;
        }
    }
    {
        SceneEvents::SunUpdated ev{}; int handled = 0;
        while (handled < 64 && sun_sub_.queue && sun_sub_.queue->try_pop(ev)) {
            scene_snapshots_[ev.sceneId].sunDir = ev.dir;
            ++handled;
        }
    }
    {
        SceneEvents::DisplaySurfaceUpdated ev{}; int handled = 0;
        while (handled < 64 && surf_sub_.queue && surf_sub_.queue->try_pop(ev)) {
            scene_snapshots_[ev.sceneId].surface = ev.surface;
            ++handled;
        }
    }
    {
        SceneEvents::Removed ev{}; int handled = 0;
        while (handled < 64 && rm_sub_.queue && rm_sub_.queue->try_pop(ev)) {
            scene_snapshots_.erase(ev.sceneId);
            ++handled;
        }
    }

    // 3) 处理 Actor 事件
    {
        ActorEvents::Spawned ev{}; int handled = 0;
        while (handled < 32 && actor_spawn_sub_.queue && actor_spawn_sub_.queue->try_pop(ev)) {
            const uint64_t id = ev.actorId;
            // 异步加载模型资源；回调中返回渲染线程处理
            Engine::instance().resources().load_once_async({"model", ev.modelPath},
                [id](const ResourceId&, std::shared_ptr<IResource> r){
                    if (!r) return;
                    auto model = std::dynamic_pointer_cast<Model>(r);
                    if (!model) return;
                    auto &rs = Engine::instance().get_system<RenderingSystem>();
                    auto &rq2 = Engine::instance().get_queue(rs.name());
                    rq2.enqueue([&rs, id, model=std::move(model)]() mutable {
                        // 若期间已收到移除事件，则不插入
                        if (rs.removed_actors_.contains(id)) return;
                        auto &mc = Engine::instance().cache<Model>();
                        mc.insert(id, model);
                        rs.watch_model(id);
                    });
                });
            ++handled;
        }
    }
    {
        ActorEvents::TransformUpdated ev{}; int handled = 0;
        while (handled < 64 && actor_tfm_sub_.queue && actor_tfm_sub_.queue->try_pop(ev)) {
            auto &trs = pending_trs_[ev.actorId];
            // 简单哨兵：零向量视为未设置（注意：pos=0 情况会被忽略，若需支持可改为事件中携带标志位）
            if (ev.pos.x != 0.f || ev.pos.y != 0.f || ev.pos.z != 0.f) trs.pos = ev.pos;
            if (ev.rot.x != 0.f || ev.rot.y != 0.f || ev.rot.z != 0.f) trs.rot = ev.rot;
            if (ev.scale.x != 0.f || ev.scale.y != 0.f || ev.scale.z != 0.f) trs.scale = ev.scale;
            ++handled;
        }
    }
    {
        ActorEvents::Removed ev{}; int handled = 0;
        while (handled < 32 && actor_rm_sub_.queue && actor_rm_sub_.queue->try_pop(ev)) {
            removed_actors_.insert(ev.actorId);
            model_cache_keys_.erase(ev.actorId);
            pending_trs_.erase(ev.actorId);
            // 从模型缓存移除（若存在）
            auto &mc = Engine::instance().cache<Model>();
            mc.erase(ev.actorId);
            ++handled;
        }
    }

    // 4) 将待应用的 TRS 刷入现有模型缓存
    if (!pending_trs_.empty()) {
        auto &mc = Engine::instance().cache<Model>();
        std::vector<uint64_t> to_clear;
        to_clear.reserve(pending_trs_.size());
        for (auto &kv : pending_trs_) {
            const uint64_t id = kv.first;
            auto data = mc.get(id);
            if (!data) continue; // 模型尚未就绪，之后再试
            auto &opt = kv.second;
            mc.modify(id, [opt](const std::shared_ptr<Model>& m){
                if (opt.pos)   m->positon = *opt.pos;
                if (opt.rot)   m->rotation = *opt.rot;
                if (opt.scale) m->scale   = *opt.scale;
            });
            to_clear.push_back(id);
        }
        for (auto id : to_clear) pending_trs_.erase(id);
    }

    // 5) 执行渲染
    update_engine();
}

void RenderingSystem::onStop()
{
    // 退订事件，释放队列
    if (cam_sub_.id) Engine::instance().events<SceneEvents::CameraUpdated>().unsubscribe(cam_sub_.topic, cam_sub_.id);
    if (sun_sub_.id) Engine::instance().events<SceneEvents::SunUpdated>().unsubscribe(sun_sub_.topic, sun_sub_.id);
    if (surf_sub_.id) Engine::instance().events<SceneEvents::DisplaySurfaceUpdated>().unsubscribe(surf_sub_.topic, surf_sub_.id);
    if (rm_sub_.id) Engine::instance().events<SceneEvents::Removed>().unsubscribe(rm_sub_.topic, rm_sub_.id);
    if (actor_spawn_sub_.id) Engine::instance().events<ActorEvents::Spawned>().unsubscribe(actor_spawn_sub_.topic, actor_spawn_sub_.id);
    if (actor_tfm_sub_.id)   Engine::instance().events<ActorEvents::TransformUpdated>().unsubscribe(actor_tfm_sub_.topic, actor_tfm_sub_.id);
    if (actor_rm_sub_.id)    Engine::instance().events<ActorEvents::Removed>().unsubscribe(actor_rm_sub_.topic, actor_rm_sub_.id);

    cam_sub_.id = sun_sub_.id = surf_sub_.id = rm_sub_.id = 0;
    actor_spawn_sub_.id = actor_tfm_sub_.id = actor_rm_sub_.id = 0;

    cam_sub_.queue.reset(); sun_sub_.queue.reset(); surf_sub_.queue.reset(); rm_sub_.queue.reset();
    actor_spawn_sub_.queue.reset(); actor_tfm_sub_.queue.reset(); actor_rm_sub_.queue.reset();
}

void RenderingSystem::init()
{
    // 初始化渲染尺寸
    gbufferSize = {1920, 1080};

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

void RenderingSystem::update_engine()
{
    if (!shaderHasInit)
        return;

    for (const auto &kv : scene_snapshots_)
    {
        const auto &snap = kv.second;
        gbuffer_pipeline(snap.camera);
        composite_pipeline(snap.sunDir);
        // 若需要把最终图像送到 snap.surface，可在此集成平台显示接口
    }
}

void RenderingSystem::gbuffer_pipeline(const CameraSnapshot& cam)
{
    uniformBufferObjects.eyePosition = cam.pos;
    uniformBufferObjects.eyeDir = ktm::normalize(cam.forward);
    uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(cam.pos, ktm::normalize(cam.forward), cam.worldUp);
    uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(cam.fov), static_cast<float>(gbufferSize.x) / static_cast<float>(gbufferSize.y), 0.1f, 100.0f);

    gbufferUniformBufferObjects.viewProjMatrix = uniformBufferObjects.eyeProjMatrix * uniformBufferObjects.eyeViewMatrix;
    gbufferUniformBuffer.copyFromData(&gbufferUniformBufferObjects, sizeof(gbufferUniformBufferObjects));

    auto &model_cache = Engine::instance().cache<Model>();
    model_cache.safe_loop_foreach(model_cache_keys_, [&](std::shared_ptr<Model> model) {
        if (!model)
            return;

        model->getModelMatrix();
        ktm::fmat4x4 actorMatrix = model->modelMatrix;
        rasterizerPipeline["pushConsts.modelMatrix"] = actorMatrix;

        rasterizerPipeline["pushConsts.uniformBufferIndex"] = gbufferUniformBuffer.storeDescriptor();

        rasterizerPipeline["gbufferPostion"] = gbufferPostionImage;
        rasterizerPipeline["gbufferBaseColor"] = gbufferBaseColorImage;
        rasterizerPipeline["gbufferNormal"] = gbufferNormalImage;
        rasterizerPipeline["gbufferMotionVector"] = gbufferMotionVectorImage;

        for (auto &m : model->meshes)
        {
            // 录制与提交绘制命令（按需开启）
            // executor(HardwareExecutor::ExecutorType::Graphics)
            //     << rasterizerPipeline(gbufferSize.x, gbufferSize.y) << rasterizerPipeline.record(m.meshDevice->indexBuffer)
            //     << executor.commit();
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
