#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/resource_manager/model.h>
#include <corona/resource_manager/resource_manager.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/optics/optics_system.h>

#include <filesystem>

#include "corona/resource_manager/shader.h"
#include "hardware.h"

#ifdef CORONA_ENABLE_VISION
#include "base/import/importer.h"
#include "base/mgr/pipeline.h"
#include "rhi/context.h"
#endif

namespace {

std::shared_ptr<Corona::Shader> load_shader(const std::filesystem::path& shader_path) {
    auto shaderId = Corona::ResourceId::from(Corona::ResourceType::TextFile, (shader_path).string());
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
        // using namespace vision;
        // using namespace ocarina;
        // visionDevice = RHIContext::instance().create_device("cuda");
        visionDevice.init_rtx();
        vision::Global::instance().set_device(&visionDevice);
        vision::Global::instance().set_scene_path("E:\\CoronaTestScenes\\test_vision\\render_scene\\kitchen");
        auto str = "E:\\CoronaTestScenes\\test_vision\\render_scene\\kitchen\\vision_scene.json";
        renderPipeline = vision::Importer::import_scene(str);
        renderPipeline->init();
        renderPipeline->prepare();
        renderPipeline->display(1 / 30);

        cudaViewBuffer = &renderPipeline->frame_buffer()->view_buffer();

        uint2 imageSize = renderPipeline->frame_buffer()->raytracing_resolution();

        imageData.resize(cudaViewBuffer->size());

        importedViewImage = HardwareImage(imageSize.x, imageSize.y, ImageFormat::RGBA32_FLOAT, ImageUsage::StorageImage);
        importedViewBuffer = HardwareBuffer(imageSize.x * imageSize.y, sizeof(float) * 4, BufferUsage::StorageBuffer);

        uint64_t viewBufferHandleWin = visionDevice.export_handle(cudaViewBuffer->handle());
        // uint64_t viewBufferHandleCUDA = visionDevice.import_handle(viewBufferHandleWin, cudaViewBuffer->size_in_byte());

        // Buffer v_buffer = visionDevice.create_buffer<float4>(cudaViewBuffer->size(), handle_ty(viewBufferHandleCUDA));

        // std::vector<float4> imageData(buffer.size());
        cudaViewBuffer->download_immediately(imageData.data());

        // std::vector<float4> imageData2(cudaViewBuffer->size());
        // v_buffer.download_immediately(imageData2.data());

        // uint2 imageSize = rp->frame_buffer()->raytracing_resolution();

        ExternalHandle handle;
        handle.handle = reinterpret_cast<HANDLE>(viewBufferHandleWin);
        // importedViewBuffer = HardwareBuffer(handle, imageSize.x * imageSize.y, sizeof(float) * 4, cudaViewBuffer->size_in_byte(), BufferUsage::StorageBuffer);

        // importedViewBuffer.copyToData(imageData2.data(), cudaViewBuffer->size_in_byte());

        // importedViewImage = HardwareImage(imageSize.x, imageSize.y, ImageFormat::RGBA32_FLOAT, ImageUsage::StorageImage);
        //  importedViewImage.copyFromBuffer(importedViewBuffer);
        //  importedViewImage.copyFromData(imageData.data());

        HardwareBuffer testBuffer = HardwareBuffer(imageSize.x * imageSize.y, sizeof(float) * 4, BufferUsage::StorageBuffer, imageData.data());

        std::vector<float4> imageData3(cudaViewBuffer->size());
        testBuffer.copyToData(imageData3.data(), cudaViewBuffer->size_in_byte());

        std::vector<float4> imageData2(cudaViewBuffer->size());
        HardwareBuffer testBuffer2(testBuffer.exportBufferMemory(), imageSize.x * imageSize.y, sizeof(float) * 4, cudaViewBuffer->size_in_byte(), BufferUsage::StorageBuffer);
        testBuffer2.copyToData(imageData2.data(), cudaViewBuffer->size_in_byte());

        std::vector<float4> imageData4(cudaViewBuffer->size());
        // HardwareBuffer testBuffer3 = HardwareBuffer(imageData3.data(), cudaViewBuffer->size_in_byte());
        // testBuffer3.copyToData(imageData4.data(), cudaViewBuffer->size_in_byte());
        // testBuffer2.copyFromData(imageData2.data(), imageData.size() * sizeof(float) * 4);imageData2
        //  importedViewImage.copyFromBuffer(testBuffer2);

        // uint64_t viewBufferHandleCUDA2 = device.import_handle(handle_ty(testBuffer.exportBufferMemory().handle), buffer.size_in_byte());

        // Buffer v_buffer = device.create_buffer<float4>(buffer.size(), handle_ty(viewBufferHandleCUDA2));

        // std::vector<float4> imageData2(buffer.size());
        // v_buffer.download_immediately(imageData2.data());

        importedViewImage.copyFromBuffer(importedViewBuffer);
#endif
    }

    auto* logger = ctx->logger();
    logger->info("OpticsSystem: Initializing...");

    hardware_ = std::make_unique<Hardware>();

    // 修正：避免聚合初始化引发的赋值问题
    hardware_->gbufferSize.x = 1920;
    hardware_->gbufferSize.y = 1080;

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

    hardware_->rasterizerPipeline = RasterizerPipeline(shader_code->vert_code, shader_code->frag_code);
    hardware_->computePipeline = ComputePipeline(shader_code->compute_code);
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
        for (auto vp_handle : scene.viewport_handles) {
            SharedDataHub::instance().viewport_storage().read(vp_handle, [&](const ViewportDevice& viewport) {
                if (viewport.camera == 0) {
                    return;
                }
                SharedDataHub::instance().camera_storage().read(viewport.camera, [&](const CameraDevice& camera) {
                    hardware_->uniformBufferObjects.eyePosition = camera.position;
                    hardware_->uniformBufferObjects.eyeDir = camera.forward;
                    hardware_->uniformBufferObjects.eyeViewMatrix = camera.compute_view_matrix();
                    hardware_->uniformBufferObjects.eyeProjMatrix = camera.compute_projection_matrix();
                    hardware_->gbufferUniformBufferObjects.viewProjMatrix = camera.compute_view_proj_matrix();
                    hardware_->gbufferUniformBuffer.copyFromData(&hardware_->gbufferUniformBufferObjects, sizeof(hardware_->gbufferUniformBufferObjects));

                    hardware_->rasterizerPipeline["gbufferPostion"] = hardware_->gbufferPostionImage;
                    hardware_->rasterizerPipeline["gbufferBaseColor"] = hardware_->gbufferBaseColorImage;
                    hardware_->rasterizerPipeline["gbufferNormal"] = hardware_->gbufferNormalImage;
                    hardware_->rasterizerPipeline["gbufferMotionVector"] = hardware_->gbufferMotionVectorImage;
                    hardware_->rasterizerPipeline.setDepthImage(hardware_->gbufferDepthImage);

                    SharedDataHub::instance().optics_storage().for_each_read([&](const OpticsDevice& optics) {
                        static HardwareBuffer s_identity_bone_buffer;
                        static bool s_identity_inited = false;
                        if (!s_identity_inited) {
                            std::vector<ktm::fmat4x4> identity_matrix = {ktm::fmat4x4::from_eye()};
                            s_identity_bone_buffer = HardwareBuffer(identity_matrix, BufferUsage::StorageBuffer);
                            s_identity_inited = true;
                        }

                        HardwareBuffer boneMatrixBuf = s_identity_bone_buffer;
                        if (optics.skinning_handle != 0) {
                            SharedDataHub::instance().skinning_storage().read(optics.skinning_handle, [&](const SkinningDevice& skin) {
                                boneMatrixBuf = skin.bone_matrix_buffer;
                            });
                        }

                        SharedDataHub::instance().geometry_storage().read(optics.geometry_handle, [&](const GeometryDevice& geom) {
                            SharedDataHub::instance().model_transform_storage().read(geom.transform_handle, [&](const ModelTransform& transform) {
                                auto model_matrix = transform.compute_matrix();
                                // 调试：输出变换数据
                                static int frame_counter = 0;
                                if (++frame_counter % 120 == 0) {  // 每2秒输出一次（假设60fps）
                                    std::cout << "Transform - pos: (" << transform.position.x << ", "
                                              << transform.position.y << ", " << transform.position.z << "), "
                                              << "geom_handle: " << optics.geometry_handle << std::endl;
                                }
                                hardware_->rasterizerPipeline["pushConsts.modelMatrix"] = model_matrix;
                            });
                            hardware_->rasterizerPipeline["pushConsts.uniformBufferIndex"] = hardware_->gbufferUniformBuffer.storeDescriptor();

                            hardware_->rasterizerPipeline["pushConsts.boneIndex"] = boneMatrixBuf.storeDescriptor();

                            for (const auto& m : geom.mesh_handles) {
                                hardware_->rasterizerPipeline["inPosition"] = m.pointsBuffer;
                                hardware_->rasterizerPipeline["inNormal"] = m.normalsBuffer;
                                hardware_->rasterizerPipeline["inTexCoord"] = m.texCoordsBuffer;
                                hardware_->rasterizerPipeline["boneIndexes"] = m.boneIndexesBuffer;
                                hardware_->rasterizerPipeline["jointWeights"] = m.boneWeightsBuffer;
                                hardware_->rasterizerPipeline["pushConsts.textureIndex"] = m.textureIndex;

                                hardware_->executor << hardware_->rasterizerPipeline.record(m.indexBuffer);
                            }
                        });
                    });

                    hardware_->computePipeline["pushConsts.gbufferSize"] = hardware_->gbufferSize;
                    hardware_->computePipeline["pushConsts.gbufferPostionImage"] = hardware_->gbufferPostionImage.storeDescriptor();
                    hardware_->computePipeline["pushConsts.gbufferBaseColorImage"] = hardware_->gbufferBaseColorImage.storeDescriptor();
                    hardware_->computePipeline["pushConsts.gbufferNormalImage"] = hardware_->gbufferNormalImage.storeDescriptor();
                    hardware_->computePipeline["pushConsts.gbufferDepthImage"] = hardware_->rasterizerPipeline.getDepthImage().storeDescriptor();

                    hardware_->computePipeline["pushConsts.finalOutputImage"] = hardware_->finalOutputImage.storeDescriptor();

                    ktm::fvec3 sun_dir;
                    sun_dir.x = 1.0f;
                    sun_dir.y = 1.0f;
                    sun_dir.z = 1.0f;
                    if (scene.environment != 0) {
                        SharedDataHub::instance().environment_storage().read(scene.environment, [&](const EnvironmentDevice& env) {
                            sun_dir = env.sun_position;
                        });
                    }

                    hardware_->computePipeline["pushConsts.sun_dir"] = ktm::normalize(sun_dir);
                    {
                        ktm::fvec3 lightColor;
                        lightColor.x = 23.47f;
                        lightColor.y = 21.31f;
                        lightColor.z = 20.79f;
                        hardware_->computePipeline["pushConsts.lightColor"] = lightColor;
                    }

                    hardware_->uniformBuffer.copyFromData(&hardware_->uniformBufferObjects, sizeof(hardware_->uniformBufferObjects));
                    hardware_->computePipeline["pushConsts.uniformBufferIndex"] = hardware_->uniformBuffer.storeDescriptor();

                    hardware_->executor << hardware_->rasterizerPipeline(1920, 1080)
                                        << hardware_->computePipeline(1920 / 8, 1080 / 8, 1)
                                        << hardware_->executor.commit();

//#ifdef CORONA_ENABLE_VISION
//                    if (hardware_->displayers_.contains(reinterpret_cast<uint64_t>(camera.surface))) {
//                        renderPipeline->display(1 / 30);
//                        cudaViewBuffer->download_immediately(imageData.data());
//                        importedViewBuffer.copyFromData(imageData.data(), imageData.size() * sizeof(float) * 4);
//                        importedViewImage.copyFromBuffer(importedViewBuffer);
//                        hardware_->displayers_.at(reinterpret_cast<uint64_t>(camera.surface)).wait(hardware_->executor) << importedViewImage;
//                    }
//#else
                    if (hardware_->displayers_.contains(reinterpret_cast<uint64_t>(camera.surface))) {
                        hardware_->displayers_.at(reinterpret_cast<uint64_t>(camera.surface)).wait(hardware_->executor) << hardware_->finalOutputImage;
                    }
//#endif
                });
            });
        }
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
