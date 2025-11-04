#pragma once

#include <CabbageHardware.h>
#include <Pipeline/ComputePipeline.h>
#include <Pipeline/RasterizerPipeline.h>


struct Hardware {
    HardwareImage gbufferPostionImage;
    HardwareImage gbufferBaseColorImage;
    HardwareImage gbufferNormalImage;
    HardwareImage gbufferMotionVectorImage;
    HardwareImage finalOutputImage;

    HardwareBuffer uniformBuffer;
    HardwareBuffer gbufferUniformBuffer;
    HardwareBuffer computeUniformBuffer;

    bool shaderHasInit = false;
    RasterizerPipeline rasterizerPipeline;
    ComputePipeline computePipeline;
    HardwareExecutor executor;

    struct UniformBufferObject {
        ktm::fvec3 eyePosition;
        float padding0;
        ktm::fvec3 eyeDir;
        float padding1;
        ktm::fmat4x4 eyeViewMatrix;
        ktm::fmat4x4 eyeProjMatrix;
    } uniformBufferObjects{};

    struct gbufferUniformBufferObject {
        ktm::fmat4x4 viewProjMatrix;
    } gbufferUniformBufferObjects{};

    struct ComputeUniformBufferObject
    {
        uint32_t imageID;
    } computeUniformBufferObjects{};

    // 渲染大小
    ktm::uvec2 gbufferSize{};

    std::unordered_map<uint64_t, HardwareDisplayer> displayers_;
};