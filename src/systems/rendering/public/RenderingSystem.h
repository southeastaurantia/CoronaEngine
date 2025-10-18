#pragma once

#include <corona/interfaces/ThreadedSystem.h>

#include <CabbageDisplayer.h>
#include <Pipeline/ComputePipeline.h>
#include <Pipeline/RasterizerPipeline.h>

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include <optional>

#include "EventBus.h"
#include "SceneEvents.h"
#include "events/ActorEvents.h"

namespace Corona
{
    class Shader;
    class Model;

    class RenderingSystem final : public ThreadedSystem
    {
      public:
        RenderingSystem();

        // 保持与现有 API 兼容的模型观测接口
        void watch_model(uint64_t id);
        void unwatch_model(uint64_t id);

        // 显式初始化着色器/管线（可通过外部事件或资源回调触发）
        void init_shader(std::shared_ptr<Shader> shader);

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        void init();
        void update_engine();

        struct CameraSnapshot {
            float fov = 45.0f;
            ktm::fvec3 pos{};
            ktm::fvec3 forward{};
            ktm::fvec3 worldUp{};
        };
        struct SceneSnapshot {
            CameraSnapshot camera{};
            ktm::fvec3 sunDir{};
            void* surface = nullptr;
        };

        struct TRS {
            std::optional<ktm::fvec3> pos;
            std::optional<ktm::fvec3> rot;
            std::optional<ktm::fvec3> scale;
        };

        void gbuffer_pipeline(const CameraSnapshot& cam);
        void composite_pipeline(ktm::fvec3 sunDir);

        // 渲染资源
        HardwareImage gbufferPostionImage;
        HardwareImage gbufferBaseColorImage;
        HardwareImage gbufferNormalImage;
        HardwareImage gbufferMotionVectorImage;
        HardwareImage finalOutputImage;

        HardwareBuffer uniformBuffer;
        HardwareBuffer gbufferUniformBuffer;

        bool shaderHasInit = false;
        RasterizerPipeline rasterizerPipeline;
        ComputePipeline computePipeline;
        HardwareExecutor executor;

        struct UniformBufferObject
        {
            ktm::fvec3 eyePosition;
            float padding0;
            ktm::fvec3 eyeDir;
            float padding1;
            ktm::fmat4x4 eyeViewMatrix;
            ktm::fmat4x4 eyeProjMatrix;
        } uniformBufferObjects{};

        struct gbufferUniformBufferObject
        {
            ktm::fmat4x4 viewProjMatrix;
        } gbufferUniformBufferObjects{};

        // 渲染大小
        ktm::uvec2 gbufferSize{};

        // 被观测的模型与场景快照
        std::unordered_set<uint64_t> model_cache_keys_{};
        std::unordered_map<uint64_t, SceneSnapshot> scene_snapshots_{};

        // Actor 事件订阅与状态
        EventBusT<SceneEvents::CameraUpdated>::Subscription cam_sub_{};
        EventBusT<SceneEvents::SunUpdated>::Subscription sun_sub_{};
        EventBusT<SceneEvents::DisplaySurfaceUpdated>::Subscription surf_sub_{};
        EventBusT<SceneEvents::Removed>::Subscription rm_sub_{};

        EventBusT<ActorEvents::Spawned>::Subscription actor_spawn_sub_{};
        EventBusT<ActorEvents::TransformUpdated>::Subscription actor_tfm_sub_{};
        EventBusT<ActorEvents::Removed>::Subscription actor_rm_sub_{};

        std::unordered_map<uint64_t, TRS> pending_trs_{};
        std::unordered_set<uint64_t> removed_actors_{};
    };
} // namespace Corona
