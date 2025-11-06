#include <ResourceManager.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/optics_system.h>
#include <corona/shared_data_hub.h>
#include <Model.h>

#include <filesystem>
#include <ranges>

#include "Shader.h"
#include "hardware.h"

namespace {

std::shared_ptr<Corona::Shader> load_shader(const std::filesystem::path& shader_path) {
    auto shaderId = Corona::ResourceId::from("shader", (shader_path).string());
    auto shader = std::static_pointer_cast<Corona::Shader>(Corona::ResourceManager::instance().load_once(shaderId));
    return shader;
}
}  // namespace

namespace Corona::Systems {

OpticsSystem::OpticsSystem() {
    set_target_fps(120);

}
OpticsSystem::~OpticsSystem() = default;

bool OpticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("OpticsSystem: Initializing...");

    hardware_ = std::make_unique<Hardware>();

    hardware_->gbufferSize = {1920, 1080};

    hardware_->gbufferPostionImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferBaseColorImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferNormalImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferMotionVectorImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RG32_FLOAT, ImageUsage::StorageImage);

    hardware_->uniformBuffer = HardwareBuffer(sizeof(Hardware::UniformBufferObject), BufferUsage::UniformBuffer);
    hardware_->gbufferUniformBuffer = HardwareBuffer(sizeof(Hardware::gbufferUniformBufferObject), BufferUsage::UniformBuffer);
    hardware_->computeUniformBuffer = HardwareBuffer(sizeof(Hardware::ComputeUniformBufferObject), BufferUsage::UniformBuffer);

    hardware_->finalOutputImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);

    auto shader_code = load_shader(std::filesystem::current_path() / "assets");

    hardware_->rasterizerPipeline = RasterizerPipeline(shader_code->vertCode, shader_code->fragCode);
    hardware_->computePipeline = ComputePipeline(shader_code->computeCode);
    hardware_->shaderHasInit = true;

    // 【订阅系统内部事件】使用 EventBus
    if (auto* event_bus = ctx->event_bus()) {
        surface_changed_sub_id_ = event_bus->subscribe<Events::DisplaySurfaceChangedEvent>(
            [this, logger](const Events::DisplaySurfaceChangedEvent& event) {
                if (logger) {
                    logger->info("OpticsSystem: Received DisplaySurfaceChangedEvent, new surface: " +
                                 std::to_string(reinterpret_cast<uintptr_t>(event.surface)));
                }
                if (event.surface) {
                    auto surface_id = reinterpret_cast<uint64_t>(event.surface);
                    if (!hardware_->displayers_.contains(surface_id)) {
                        logger->info("OpticsSystem: Creating new displayer for surface " + std::to_string(surface_id));
                        hardware_->displayers_.emplace(surface_id, HardwareDisplayer(event.surface));
                    }
                }
            });
        logger->info("OpticsSystem: Subscribed to DisplaySurfaceChangedEvent");
    }

    return true;
}

void OpticsSystem::update() {
    if (!hardware_->shaderHasInit) {
        return;
    }

    static float frame_count = 0.0f;
    float dt = delta_time();
    frame_count += dt;

    for (auto& displayer : hardware_->displayers_ | std::views::values) {

        hardware_->computeUniformBufferObjects.imageID = hardware_->finalOutputImage.storeDescriptor();
        hardware_->computeUniformBufferObjects.imageSize = hardware_->gbufferSize;
        hardware_->computeUniformBufferObjects.time = frame_count;

        hardware_->computeUniformBuffer.copyFromData(&hardware_->computeUniformBufferObjects, sizeof(hardware_->computeUniformBufferObjects));
        hardware_->computePipeline["pushConsts.uniformBufferIndex"] = hardware_->computeUniformBuffer.storeDescriptor();

        hardware_->executor
            // << hardware_->rasterizerPipeline(1920, 1080)
            << hardware_->computePipeline(1920 / 8, 1080 / 8, 1)
            << hardware_->executor.commit();

        displayer = hardware_->finalOutputImage;
    }
}

void OpticsSystem::gbuffer_pipeline() {
    // uniformBufferObjects.eyePosition = cam.pos;
    // uniformBufferObjects.eyeDir = ktm::normalize(cam.forward);
    // uniformBufferObjects.eyeViewMatrix = ktm::look_at_lh(cam.pos, ktm::normalize(cam.forward), cam.worldUp);
    // uniformBufferObjects.eyeProjMatrix = ktm::perspective_lh(ktm::radians(cam.fov), static_cast<float>(gbufferSize.x) / static_cast<float>(gbufferSize.y), 0.1f, 100.0f);

    hardware_->gbufferUniformBufferObjects.viewProjMatrix = hardware_->uniformBufferObjects.eyeProjMatrix * hardware_->uniformBufferObjects.eyeViewMatrix;
    hardware_->gbufferUniformBuffer.copyFromData(&hardware_->gbufferUniformBufferObjects, sizeof(hardware_->gbufferUniformBufferObjects));

    // model->getModelMatrix();
    // ktm::fmat4x4 actorMatrix = model->modelMatrix;
    // hardware_->rasterizerPipeline["pushConsts.modelMatrix"] = actorMatrix;

    hardware_->rasterizerPipeline["pushConsts.uniformBufferIndex"] = hardware_->gbufferUniformBuffer.storeDescriptor();

    hardware_->rasterizerPipeline["gbufferPostion"] = hardware_->gbufferPostionImage;
    hardware_->rasterizerPipeline["gbufferBaseColor"] = hardware_->gbufferBaseColorImage;
    hardware_->rasterizerPipeline["gbufferNormal"] = hardware_->gbufferNormalImage;
    hardware_->rasterizerPipeline["gbufferMotionVector"] = hardware_->gbufferMotionVectorImage;

    // for () {
    //     // 录制与提交绘制命令（按需开启）
    //     // executor(HardwareExecutor::ExecutorType::Graphics)
    //     //     << rasterizerPipeline(gbufferSize.x, gbufferSize.y) << rasterizerPipeline.record(m.meshDevice->indexBuffer)
    //     //     << executor.commit();
    // }

}


void OpticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("OpticsSystem: Shutting down...");

    // 取消 EventBus 订阅
    if (auto* event_bus = context()->event_bus()) {
        if (surface_changed_sub_id_ != 0) {
            event_bus->unsubscribe(surface_changed_sub_id_);
        }
    }

    hardware_.reset();
}

}  // namespace Corona::Systems
