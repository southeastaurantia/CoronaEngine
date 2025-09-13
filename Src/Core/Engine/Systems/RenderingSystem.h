#pragma once

#include "CabbageDisplayer.h"
#include "Core/Engine/ThreadedSystem.h"
#include "Pipeline/ComputePipeline.h"
#include "Pipeline/RasterizerPipeline.h"
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

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        // 迁移保留：全局DataCache的所有key集合（后续用于 foreach）
        std::unordered_set<uint64_t> data_keys_{};

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

        ktm::uvec2 gbufferSize = ktm::uvec2(1920, 1080);
        HardwareImage gbufferPostionImage;
        HardwareImage gbufferBaseColorImage;
        HardwareImage gbufferNormalImage;
        HardwareImage gbufferMotionVectorImage;

        // RasterizerPipeline gbufferPipeline;
        // ComputePipeline compositePipeline;

        HardwareBuffer uniformBuffer;
        HardwareBuffer gbufferUniformBuffer;

        // 对应旧版的渲染流程拆分
        void init();
        void processRender(uint64_t id);
        void updateEngine();
        void gbufferPipeline();
        void compositePipeline();
    };
} // namespace Corona
