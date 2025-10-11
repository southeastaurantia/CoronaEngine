#pragma once

#include "core/engine/ThreadedSystem.h"
#include "resource/Scene.h"

#include <CabbageDisplayer.h>
#include <Pipeline/ComputePipeline.h>
#include <Pipeline/RasterizerPipeline.h>

#include <memory>
#include <unordered_set>

namespace Corona
{
    class Shader;
    class Model;

    class RenderingSystem final : public ThreadedSystem
    {
      public:
        RenderingSystem();

        void WatchModel(uint64_t id);
        void UnwatchModel(uint64_t id);
        void WatchScene(uint64_t id);
        void UnwatchScene(uint64_t id);
        void ClearWatched();

        void initShader(std::shared_ptr<Shader> shader);
        void setDisplaySurface(std::shared_ptr<Scene> scene);

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        void init();
        void updateEngine();
        void gbufferPipeline(std::shared_ptr<Scene> scene);
        void compositePipeline(ktm::fvec3 sunDir = ktm::normalize(ktm::fvec3(-0.2f, -1.0f, -0.3f)));

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

        ktm::fvec2 gbufferSize = {1920.0f, 1080.0f};
        std::unordered_set<uint64_t> model_cache_keys_{};
        std::unordered_set<uint64_t> scene_cache_keys_{};
    };
} // namespace Corona
