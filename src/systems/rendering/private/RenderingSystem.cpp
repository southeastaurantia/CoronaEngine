#include "RenderingSystem.h"

#include "Model.h"
#include "Shader.h"
#include "SystemHubs.h"

#include <memory>
#include <filesystem>

#include <ResourceTypes.h>

using namespace Corona;

RenderingSystem::RenderingSystem()
    : ThreadedSystem("RenderingSystem") {}

void RenderingSystem::configure(const Interfaces::SystemContext &context)
{
    ThreadedSystem::configure(context);
    resource_service_ = services().try_get<Interfaces::IResourceService>();
    scheduler_ = services().try_get<Interfaces::ICommandScheduler>();
    event_hub_ptr_ = event_hub();
    data_cache_hub_ = data_caches();
    if (scheduler_)
    {
        system_queue_handle_ = scheduler_->get_queue(name());
        if (!system_queue_handle_)
        {
            system_queue_handle_ = scheduler_->create_queue(name());
        }
    }
}

void RenderingSystem::onStart()
{
    init();

    if (event_hub_ptr_)
    {
        cam_sub_ = event_hub_ptr_->get<SceneEvents::CameraUpdated>().subscribe("scene.camera");
        sun_sub_ = event_hub_ptr_->get<SceneEvents::SunUpdated>().subscribe("scene.sun");
        surf_sub_ = event_hub_ptr_->get<SceneEvents::DisplaySurfaceUpdated>().subscribe("scene.surface");
        rm_sub_ = event_hub_ptr_->get<SceneEvents::Removed>().subscribe("scene.removed");

        actor_spawn_sub_ = event_hub_ptr_->get<ActorEvents::Spawned>().subscribe("actor.spawn");
        actor_tfm_sub_   = event_hub_ptr_->get<ActorEvents::TransformUpdated>().subscribe("actor.transform");
        actor_rm_sub_    = event_hub_ptr_->get<ActorEvents::Removed>().subscribe("actor.removed");
    }
    else
    {
        CE_LOG_WARN("[RenderingSystem] EventBusHub 未注入，事件订阅被跳过");
    }

    if (!resource_service_)
    {
        CE_LOG_WARN("[RenderingSystem] 资源服务未注册，跳过示例加载");
        return;
    }
    if (!system_queue_handle_)
    {
        CE_LOG_WARN("[RenderingSystem] 命令队列句柄缺失，跳过示例加载");
        return;
    }

    const auto assets_root = (std::filesystem::current_path() / "assets").string();
    auto shaderId = ResourceId::from("shader", assets_root);
    auto queue_handle = system_queue_handle_;
    resource_service_->load_once_async(
        shaderId,
        [queue_handle](const ResourceId&, std::shared_ptr<IResource> r) {
            if (!queue_handle)
            {
                return;
            }
            queue_handle->enqueue([success = static_cast<bool>(r)] {
                if (!success)
                {
                    CE_LOG_WARN("[RenderingSystem] 异步加载 shader 失败");
                    return;
                }
                CE_LOG_INFO("[RenderingSystem] 异步加载 shader 成功（测试样例）");
            });
        });
}

void RenderingSystem::onTick()
{
    if (auto *queue = command_queue())
    {
        int spun = 0;
        while (spun < 128 && !queue->empty())
        {
            if (!queue->try_execute())
            {
                continue;
            }
            ++spun;
        }
    }

    {
        SceneEvents::CameraUpdated ev{};
        int handled = 0;
        while (handled < 64 && cam_sub_.queue && cam_sub_.queue->try_pop(ev))
        {
            auto &snap = scene_snapshots_[ev.sceneId];
            snap.camera.fov = ev.fov;
            snap.camera.pos = ev.pos;
            snap.camera.forward = ev.forward;
            snap.camera.worldUp = ev.worldUp;
            ++handled;
        }
    }
    {
        SceneEvents::SunUpdated ev{};
        int handled = 0;
        while (handled < 64 && sun_sub_.queue && sun_sub_.queue->try_pop(ev))
        {
            scene_snapshots_[ev.sceneId].sunDir = ev.dir;
            ++handled;
        }
    }
    {
        SceneEvents::DisplaySurfaceUpdated ev{};
        int handled = 0;
        while (handled < 64 && surf_sub_.queue && surf_sub_.queue->try_pop(ev))
        {
            scene_snapshots_[ev.sceneId].surface = ev.surface;
            ++handled;
        }
    }
    {
        SceneEvents::Removed ev{};
        int handled = 0;
        while (handled < 64 && rm_sub_.queue && rm_sub_.queue->try_pop(ev))
        {
            scene_snapshots_.erase(ev.sceneId);
            ++handled;
        }
    }

    auto *caches = data_cache_hub_;

    if (resource_service_ && system_queue_handle_)
    {
        ActorEvents::Spawned ev{};
        int handled = 0;
        while (handled < 32 && actor_spawn_sub_.queue && actor_spawn_sub_.queue->try_pop(ev))
        {
            const uint64_t id = ev.actorId;
            auto queue_handle = system_queue_handle_;
            auto cache_hub = caches;
            auto self = this;
            resource_service_->load_once_async(
                ResourceId::from("model", ev.modelPath),
                [queue_handle, cache_hub, self, id](const ResourceId &, std::shared_ptr<IResource> r) mutable {
                    if (!queue_handle)
                    {
                        return;
                    }
                    queue_handle->enqueue([queue_handle, cache_hub, self, id, resource = std::move(r)]() mutable {
                        auto model = std::dynamic_pointer_cast<Model>(resource);
                        if (!model || !self)
                        {
                            return;
                        }
                        if (self->removed_actors_.contains(id))
                        {
                            return;
                        }
                        if (!cache_hub)
                        {
                            return;
                        }
                        auto &mc = cache_hub->get<Model>();
                        mc.insert(id, model);
                        self->watch_model(id);
                    });
                });
            ++handled;
        }
    }

    {
        ActorEvents::TransformUpdated ev{};
        int handled = 0;
        while (handled < 64 && actor_tfm_sub_.queue && actor_tfm_sub_.queue->try_pop(ev))
        {
            auto &trs = pending_trs_[ev.actorId];
            if (ev.pos.x != 0.f || ev.pos.y != 0.f || ev.pos.z != 0.f)
            {
                trs.pos = ev.pos;
            }
            if (ev.rot.x != 0.f || ev.rot.y != 0.f || ev.rot.z != 0.f)
            {
                trs.rot = ev.rot;
            }
            if (ev.scale.x != 0.f || ev.scale.y != 0.f || ev.scale.z != 0.f)
            {
                trs.scale = ev.scale;
            }
            ++handled;
        }
    }

    if (caches)
    {
        ActorEvents::Removed ev{};
        int handled = 0;
        while (handled < 32 && actor_rm_sub_.queue && actor_rm_sub_.queue->try_pop(ev))
        {
            removed_actors_.insert(ev.actorId);
            model_cache_keys_.erase(ev.actorId);
            pending_trs_.erase(ev.actorId);
            auto &mc = caches->get<Model>();
            mc.erase(ev.actorId);
            ++handled;
        }
    }

    if (caches && !pending_trs_.empty())
    {
        auto &mc = caches->get<Model>();
        std::vector<uint64_t> to_clear;
        to_clear.reserve(pending_trs_.size());
        for (auto &kv : pending_trs_)
        {
            const uint64_t id = kv.first;
            auto data = mc.get(id);
            if (!data)
            {
                continue;
            }
            auto opt = kv.second;
            mc.modify(id, [opt](const std::shared_ptr<Model> &m) {
                if (opt.pos)
                {
                    m->positon = *opt.pos;
                }
                if (opt.rot)
                {
                    m->rotation = *opt.rot;
                }
                if (opt.scale)
                {
                    m->scale = *opt.scale;
                }
            });
            to_clear.push_back(id);
        }
        for (auto id : to_clear)
        {
            pending_trs_.erase(id);
        }
    }

    update_engine();
}

void RenderingSystem::onStop()
{
    if (event_hub_ptr_)
    {
        if (cam_sub_.id)
        {
            event_hub_ptr_->get<SceneEvents::CameraUpdated>().unsubscribe(cam_sub_.topic, cam_sub_.id);
        }
        if (sun_sub_.id)
        {
            event_hub_ptr_->get<SceneEvents::SunUpdated>().unsubscribe(sun_sub_.topic, sun_sub_.id);
        }
        if (surf_sub_.id)
        {
            event_hub_ptr_->get<SceneEvents::DisplaySurfaceUpdated>().unsubscribe(surf_sub_.topic, surf_sub_.id);
        }
        if (rm_sub_.id)
        {
            event_hub_ptr_->get<SceneEvents::Removed>().unsubscribe(rm_sub_.topic, rm_sub_.id);
        }
        if (actor_spawn_sub_.id)
        {
            event_hub_ptr_->get<ActorEvents::Spawned>().unsubscribe(actor_spawn_sub_.topic, actor_spawn_sub_.id);
        }
        if (actor_tfm_sub_.id)
        {
            event_hub_ptr_->get<ActorEvents::TransformUpdated>().unsubscribe(actor_tfm_sub_.topic, actor_tfm_sub_.id);
        }
        if (actor_rm_sub_.id)
        {
            event_hub_ptr_->get<ActorEvents::Removed>().unsubscribe(actor_rm_sub_.topic, actor_rm_sub_.id);
        }
    }

    cam_sub_.id = sun_sub_.id = surf_sub_.id = rm_sub_.id = 0;
    actor_spawn_sub_.id = actor_tfm_sub_.id = actor_rm_sub_.id = 0;

    cam_sub_.queue.reset();
    sun_sub_.queue.reset();
    surf_sub_.queue.reset();
    rm_sub_.queue.reset();
    actor_spawn_sub_.queue.reset();
    actor_tfm_sub_.queue.reset();
    actor_rm_sub_.queue.reset();
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
    {
        return;
    }

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

    auto *caches = data_cache_hub_;
    if (!caches)
    {
        return;
    }
    auto &model_cache = caches->get<Model>();
    model_cache.safe_loop_foreach(model_cache_keys_, [&](std::shared_ptr<Model> model) {
        if (!model)
        {
            return;
        }

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
