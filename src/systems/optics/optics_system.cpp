#include <Model.h>
#include <ResourceManager.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/optics/optics_system.h>

#include <filesystem>
#include <ranges>

#include "Shader.h"
#include "hardware.h"

#ifdef CORONA_ENABLE_VISION
#include "base/import/importer.h"
#include "base/mgr/pipeline.h"
#include "rhi/context.h"
#endif

namespace {

std::shared_ptr<Corona::Shader> load_shader(const std::filesystem::path& shader_path) {
    auto shaderId = Corona::ResourceId::from("shader", (shader_path).string());
    auto shader = std::static_pointer_cast<Corona::Shader>(Corona::ResourceManager::instance().load_once(shaderId));
    return shader;
}

#ifdef CORONA_ENABLE_VISION
RegistrableBuffer<float4>* cudaViewBuffer;
HardwareBuffer importedViewBuffer;
HardwareImage importedViewImage;
std::vector<float4> imageData;
SP<vision::Pipeline> renderPipeline;
vision::Device visionDevice = RHIContext::instance().create_device("cuda");
#endif
}  // namespace

namespace Corona::Systems {

OpticsSystem::OpticsSystem() {
    set_target_fps(120);
}
OpticsSystem::~OpticsSystem() = default;

bool OpticsSystem::initialize(Kernel::ISystemContext* ctx) {
    {
#ifdef CORONA_ENABLE_VISION
         //using namespace vision;
         //using namespace ocarina;
         //visionDevice = RHIContext::instance().create_device("cuda");
         visionDevice.init_rtx();
         vision::Global::instance().set_device(&visionDevice);
         vision::Global::instance().set_scene_path("E:\\CoronaResource\\examples\\assets\\test_vision\\render_scene\\kitchen");
         auto str = "E:\\CoronaResource\\examples\\assets\\test_vision\\render_scene\\kitchen\\vision_scene.json";
         renderPipeline = vision::Importer::import_scene(str);
         renderPipeline->init();
         renderPipeline->prepare();
         renderPipeline->display(1 / 30);

         cudaViewBuffer = &renderPipeline->frame_buffer()->view_buffer();

         uint2 imageSize = renderPipeline->frame_buffer()->raytracing_resolution();

         imageData.resize(cudaViewBuffer->size());

         importedViewImage = HardwareImage(imageSize.x, imageSize.y, ImageFormat::RGBA32_FLOAT, ImageUsage::StorageImage);
         importedViewBuffer = HardwareBuffer(imageSize.x * imageSize.y, sizeof(float) * 4, BufferUsage::StorageBuffer);

         //uint64_t viewBufferHandleWin = device.export_handle(*buffer.handle());
         //uint64_t viewBufferHandleCUDA = device.import_handle(viewBufferHandleWin, buffer.size_in_byte());

         // Buffer v_buffer = device.create_buffer<float4>(buffer.size(), handle_ty(viewBufferHandleCUDA));

         //std::vector<float4> imageData(buffer.size());
         //buffer.download_immediately(imageData.data());

         // std::vector<float4> imageData2(buffer.size());
         // v_buffer.download_immediately(imageData2.data());

         //uint2 imageSize = rp->frame_buffer()->raytracing_resolution();

          //ExternalHandle handle;
          //handle.handle = reinterpret_cast<HANDLE>(viewBufferHandleWin);
          //importedViewBuffer = HardwareBuffer(handle, imageSize.x * imageSize.y, sizeof(float) * 4, buffer.size_in_byte(), BufferUsage::StorageBuffer);

         //importedViewImage = HardwareImage(imageSize.x, imageSize.y, ImageFormat::RGBA32_FLOAT, ImageUsage::StorageImage);
         // importedViewImage.copyFromBuffer(importedViewBuffer);
         // importedViewImage.copyFromData(imageData.data());

         //HardwareBuffer testBuffer = HardwareBuffer(imageSize.x * imageSize.y, sizeof(float) * 4, BufferUsage::StorageBuffer, imageData.data());
         // HardwareBuffer testBuffer2(testBuffer.exportBufferMemory(), imageSize.x * imageSize.y, sizeof(float) * 4, buffer.size_in_byte(), BufferUsage::StorageBuffer);
         // testBuffer2.copyFromData(imageData.data(), imageData.size() * sizeof(float) * 4);

         // importedViewImage.copyFromBuffer(testBuffer2);

         //uint64_t viewBufferHandleCUDA2 = device.import_handle(handle_ty(testBuffer.exportBufferMemory().handle), buffer.size_in_byte());

         //Buffer v_buffer = device.create_buffer<float4>(buffer.size(), handle_ty(viewBufferHandleCUDA2));

         //std::vector<float4> imageData2(buffer.size());
         //v_buffer.download_immediately(imageData2.data());

         //importedViewImage.copyFromBuffer(importedViewBuffer);
#endif
    }

    auto* logger = ctx->logger();
    logger->info("OpticsSystem: Initializing...");

    hardware_ = std::make_unique<Hardware>();

    hardware_->gbufferSize = {1920, 1080};

    hardware_->gbufferPostionImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferBaseColorImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferNormalImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RGBA16_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferMotionVectorImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::RG32_FLOAT, ImageUsage::StorageImage);
    hardware_->gbufferDepthImage = HardwareImage(hardware_->gbufferSize.x, hardware_->gbufferSize.y, ImageFormat::D32_FLOAT, ImageUsage::DepthImage);

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

    if (!hardware_->displayers_.empty()) {
        optics_pipeline(frame_count);
    }
}

void OpticsSystem::optics_pipeline(float frame_count) const {
    SharedDataHub::instance().scene_storage().for_each_read([&](const SceneDevice& scene) {
        SharedDataHub::instance().camera_storage().for_each_read([&](const CameraDevice& camera) {
            hardware_->uniformBufferObjects.eyePosition = camera.eye_position;
            hardware_->uniformBufferObjects.eyeDir = camera.eye_dir;
            hardware_->uniformBufferObjects.eyeViewMatrix = camera.eye_view_matrix;
            hardware_->uniformBufferObjects.eyeProjMatrix = camera.eye_proj_matrix;
            hardware_->gbufferUniformBufferObjects.viewProjMatrix = camera.view_proj_matrix;
            hardware_->gbufferUniformBuffer.copyFromData(&hardware_->gbufferUniformBufferObjects, sizeof(hardware_->gbufferUniformBufferObjects));

            hardware_->rasterizerPipeline["gbufferPostion"] = hardware_->gbufferPostionImage;
            hardware_->rasterizerPipeline["gbufferBaseColor"] = hardware_->gbufferBaseColorImage;
            hardware_->rasterizerPipeline["gbufferNormal"] = hardware_->gbufferNormalImage;
            hardware_->rasterizerPipeline["gbufferMotionVector"] = hardware_->gbufferMotionVectorImage;
            hardware_->rasterizerPipeline.setDepthImage(hardware_->gbufferDepthImage);

            SharedDataHub::instance().model_device_storage().for_each_read([&](const ModelDevice& model) {
                bool info = SharedDataHub::instance().model_transform_storage().read(model.transform_handle, [&](const ModelTransform& transform) {
                    hardware_->rasterizerPipeline["pushConsts.modelMatrix"] = transform.model_matrix;
                });
                hardware_->rasterizerPipeline["pushConsts.uniformBufferIndex"] = hardware_->gbufferUniformBuffer.storeDescriptor();

                HardwareBuffer boneMatrix = model.bone_matrix_buffer;
                hardware_->rasterizerPipeline["pushConsts.boneIndex"] = boneMatrix.storeDescriptor();

                for (auto& m : model.devices) {
                    hardware_->rasterizerPipeline["inPosition"] = m.pointsBuffer;
                    hardware_->rasterizerPipeline["inNormal"] = m.normalsBuffer;
                    hardware_->rasterizerPipeline["inTexCoord"] = m.texCoordsBuffer;
                    hardware_->rasterizerPipeline["boneIndexes"] = m.boneIndexesBuffer;
                    hardware_->rasterizerPipeline["jointWeights"] = m.boneWeightsBuffer;
                    hardware_->rasterizerPipeline["pushConsts.textureIndex"] = m.textureIndex;

                    hardware_->executor << hardware_->rasterizerPipeline.record(m.indexBuffer);
                }
            });

            hardware_->computePipeline["pushConsts.gbufferSize"] = hardware_->gbufferSize;
            hardware_->computePipeline["pushConsts.gbufferPostionImage"] = hardware_->gbufferPostionImage.storeDescriptor();
            hardware_->computePipeline["pushConsts.gbufferBaseColorImage"] = hardware_->gbufferBaseColorImage.storeDescriptor();
            hardware_->computePipeline["pushConsts.gbufferNormalImage"] = hardware_->gbufferNormalImage.storeDescriptor();
            hardware_->computePipeline["pushConsts.gbufferDepthImage"] = hardware_->rasterizerPipeline.getDepthImage().storeDescriptor();

            hardware_->computePipeline["pushConsts.finalOutputImage"] = hardware_->finalOutputImage.storeDescriptor();

            hardware_->computePipeline["pushConsts.sun_dir"] = ktm::normalize(scene.sun_direction);
            hardware_->computePipeline["pushConsts.lightColor"] = ktm::fvec3{23.47f, 21.31f, 20.79f};

            hardware_->uniformBuffer.copyFromData(&hardware_->uniformBufferObjects, sizeof(hardware_->uniformBufferObjects));
            hardware_->computePipeline["pushConsts.uniformBufferIndex"] = hardware_->uniformBuffer.storeDescriptor();

            if (!SharedDataHub::instance().model_device_storage().empty()) {
                hardware_->executor << hardware_->rasterizerPipeline(1920, 1080)
                                    << hardware_->executor.commit();
            }

            hardware_->executor
                << hardware_->computePipeline(1920 / 8, 1080 / 8, 1)
                << hardware_->executor.commit();
#ifdef CORONA_ENABLE_VISION
            if (hardware_->displayers_.contains(camera.surface)) {
                renderPipeline->display(1 / 30);
                cudaViewBuffer->download_immediately(imageData.data());
                importedViewBuffer.copyFromData(imageData.data(), imageData.size() * sizeof(float) * 4);
                importedViewImage.copyFromBuffer(importedViewBuffer);
                hardware_->displayers_.at(camera.surface).wait(hardware_->executor) << importedViewImage;
            }
#else
            if (hardware_->displayers_.contains(camera.surface)) {
                hardware_->displayers_.at(camera.surface).wait(hardware_->executor) << hardware_->finalOutputImage;
            }
#endif
        });
    });
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
