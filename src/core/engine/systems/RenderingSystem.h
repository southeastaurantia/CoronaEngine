#pragma once

#include "CabbageDisplayer.h"
#include "Core/Engine/ThreadedSystem.h"
#include "Pipeline/ComputePipeline.h"
#include "Pipeline/RasterizerPipeline.h"
#include "Resource/Scene.h"
#include "Resource/Shader.h"
#include "ktm/type_vec.h"

#include <unordered_set>

namespace Corona
{
    class RenderingSystem final : public ThreadedSystem
    {
      public:
        RenderingSystem();

        // 向渲染系统队列投递数据关注/取消命令
        void WatchModel(uint64_t id);
        void UnwatchModel(uint64_t id);
        void WatchScene(uint64_t id);
        void UnwatchScene(uint64_t id);
        void ClearWatched(); // 清空关注集合

        void setDisplaySurface(std::shared_ptr<Scene> scene);
        void initShader(std::shared_ptr<Shader> shader);

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        // 迁移保留：全局DataCache的所有key集合（后续用于 foreach）
        std::unordered_set<uint64_t> model_cache_keys_{};
        std::unordered_set<uint64_t> scene_cache_keys_{};

        bool shaderHasInit = false;
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

        struct UniformBufferObject
        {
            ktm::fvec3 lightPostion;
            ktm::fmat4x4 lightViewMatrix;
            ktm::fmat4x4 lightProjMatrix;

            ktm::fvec3 eyePosition;
            ktm::fvec3 eyeDir;
            ktm::fmat4x4 eyeViewMatrix;
            ktm::fmat4x4 eyeProjMatrix;
        } uniformBufferObjects;

        struct gbufferUniformBufferObject
        {
            ktm::fmat4x4 viewProjMatrix;
        } gbufferUniformBufferObjects;

        ktm::uvec2 gbufferSize = ktm::uvec2(800, 800);
        HardwareImage gbufferPostionImage;
        HardwareImage gbufferBaseColorImage;
        HardwareImage gbufferNormalImage;
        HardwareImage gbufferMotionVectorImage;

        RasterizerPipeline rasterizerPipeline;
        ComputePipeline computePipeline;

        HardwareImage finalOutputImage;

        HardwareBuffer uniformBuffer;
        HardwareBuffer gbufferUniformBuffer;

        HardwareExecutor executor;

        // 对应旧版的渲染流程拆分
        void init();
        void updateEngine();
        void gbufferPipeline(std::shared_ptr<Scene> scene);
        void compositePipeline(ktm::fvec3 sunDir = ktm::fvec3(0.0, 1.0, 0.0));
    };
} // namespace Corona
