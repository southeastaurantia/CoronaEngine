#pragma once

#include "CabbageDisplayer.h"
#include "Core/Engine/ThreadedSystem.h"
#include "Pipeline/ComputePipeline.h"
#include "Pipeline/RasterizerPipeline.h"
#include "Resource/Shader.h"
#include "ktm/type_vec.h"

#include <unordered_set>

namespace Corona
{
    class RenderingSystem final : public ThreadedSystem
    {
      public:
        RenderingSystem();

        // 提供静态便捷方法，向渲染系统队列投递数据关注/取消命令
        static void WatchMesh(uint64_t id);   // 关注某个 Mesh 数据 id
        static void UnwatchMesh(uint64_t id); // 取消关注
        static void ClearWatched();           // 清空关注集合
        static void setDisplaySurface(void *surface);
        void initShader(std::shared_ptr<Shader> shader);

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        // 迁移保留：全局DataCache的所有key集合（后续用于 foreach）
        std::unordered_set<uint64_t> data_keys_{};
        std::vector<std::unique_ptr<HardwareDisplayer>> displayers_{};

        bool shaderHasInit = false;
        std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

        struct RasterizerUniformBufferObject
        {
            uint32_t textureIndex;
            ktm::fmat4x4 model = ktm::rotate3d_axis(ktm::radians(90.0f), ktm::fvec3(0.0f, 0.0f, 1.0f));
            ktm::fmat4x4 view = ktm::look_at_lh(ktm::fvec3(2.0f, 2.0f, 2.0f), ktm::fvec3(0.0f, 0.0f, 0.0f), ktm::fvec3(0.0f, 0.0f, 1.0f));
            ktm::fmat4x4 proj = ktm::perspective_lh(ktm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 10.0f);
            ktm::fvec3 viewPos = ktm::fvec3(2.0f, 2.0f, 2.0f);
            ktm::fvec3 lightColor = ktm::fvec3(10.0f, 10.0f, 10.0f);
            ktm::fvec3 lightPos = ktm::fvec3(1.0f, 1.0f, 1.0f);
        }rasterizerUniformBufferObject;

        struct ComputeUniformBufferObject
        {
            uint32_t imageID;
            uint32_t _pad0[3] = {0, 0, 0};
            ktm::fvec4 sunParams0 = ktm::fvec4(0.6f, 0.6f, 0.12f, 0.0f); // x,y = NDC center; z = radius in NDC
            ktm::fvec4 sunColor = ktm::fvec4(8.0f, 7.0f, 5.0f, 0.0f);   // HDR radiance (pre-tonemap)
        }computeUniformData;

        ktm::uvec2 gbufferSize = ktm::uvec2(1920, 1080);
        HardwareImage gbufferPostionImage;
        HardwareImage gbufferBaseColorImage;
        HardwareImage gbufferNormalImage;
        HardwareImage gbufferMotionVectorImage;

        RasterizerPipeline rasterizerPipeline;
        ComputePipeline computePipeline;

        HardwareImage finalOutputImage;

        HardwareBuffer computeUniformBuffer;
        HardwareBuffer rasterizerUniformBuffer;

        // 对应旧版的渲染流程拆分
        void init();
        void updateEngine();
        void gbufferPipeline();
        void compositePipeline();
    };
} // namespace Corona
