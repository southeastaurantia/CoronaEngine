//
// Created by 25473 on 25-9-19.
//

#include <CabbageHardware.h>
#include <ResourceManager.h>
#include <corona/components/actor_components.h>
#include <corona/components/scene_components.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/kernel_context.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/script/api/corona_engine_api.h>
#include <corona/shared_data_hub.h>

#include "Model.h"

entt::registry CoronaEngineAPI::registry_;

// ########################
//          Scene
// ########################
CoronaEngineAPI::Scene::Scene(bool lightField)
    : scene_id_(registry_.create()) {
    registry_.emplace<RenderTag>(scene_id_);

    scene_handle_ = Corona::SharedDataHub::instance().scene_storage().allocate([&](Corona::SceneDevice& slot) {
        slot.sun_direction.x = 0.0f;
        slot.sun_direction.y = -1.0f;
        slot.sun_direction.z = 0.0f;
    });
}

CoronaEngineAPI::Scene::~Scene() {
    if (scene_handle_ != 0) {
        Corona::SharedDataHub::instance().scene_storage().deallocate(scene_handle_);
    }
    registry_.destroy(scene_id_);
}

void CoronaEngineAPI::Scene::set_sun_direction(ktm::fvec3 direction) const {
    registry_.emplace_or_replace<Corona::Components::SunDirection>(scene_id_, Corona::Components::SunDirection{direction});
    bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
        slot.sun_direction = direction;
    });
    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::set_sun_direction] 更新场景数据失败");
        }
    }
}

void CoronaEngineAPI::Scene::add_camera(const Camera& camera) const {
    auto& storage = registry_.get_or_emplace<Corona::Components::Storage>(scene_id_);
    storage.cameras.push_back(camera.get_id());

    std::uintptr_t handle = camera.get_handle_id();
    if (handle == 0) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::add_camera] Camera 句柄无效，跳过添加");
        }
        return;
    }

    bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
        slot.cameras.push_back(handle);
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::add_camera] 更新场景数据失败");
        }
    }
}

void CoronaEngineAPI::Scene::add_light(const Light& light) const {
    auto& storage = registry_.get_or_emplace<Corona::Components::Storage>(scene_id_);
    storage.lights.push_back(light.get_id());

    std::uintptr_t handle = light.get_handle_id();
    if (handle == 0) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::add_light] Light 句柄无效，跳过添加");
        }
        return;
    }

    bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
        slot.lights.push_back(handle);
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::add_light] 更新场景数据失败");
        }
    }
}

void CoronaEngineAPI::Scene::add_actor(const Actor& actor) const {
    auto& storage = registry_.get_or_emplace<Corona::Components::Storage>(scene_id_);
    storage.actors.push_back(actor.get_id());

    std::uintptr_t handle = actor.get_handle_id();
    if (handle == 0) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::add_actor] Actor 句柄无效，跳过添加");
        }
        return;
    }

    bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
        slot.actors.push_back(handle);
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Scene::add_actor] 更新场景数据失败");
        }
    }
}

void CoronaEngineAPI::Scene::remove_camera(const Camera& camera) const {
    auto& storage = registry_.get_or_emplace<Corona::Components::Storage>(scene_id_);
    auto cam_id = camera.get_id();
    std::erase(storage.cameras, cam_id);

    std::uintptr_t handle = camera.get_handle_id();
    if (handle != 0) {
        bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
            std::erase(slot.cameras, handle);
        });

        if (!info) {
            if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
                logger->warning("[CoronaEngineAPI::Scene::remove_camera] 更新场景数据失败");
            }
        }
    }
}

void CoronaEngineAPI::Scene::remove_light(const Light& light) const {
    auto& storage = registry_.get_or_emplace<Corona::Components::Storage>(scene_id_);
    auto light_id = light.get_id();
    std::erase(storage.lights, light_id);

    std::uintptr_t handle = light.get_handle_id();
    if (handle != 0) {
        bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
            std::erase(slot.lights, handle);
        });

        if (!info) {
            if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
                logger->warning("[CoronaEngineAPI::Scene::remove_light] 更新场景数据失败");
            }
        }
    }
}

void CoronaEngineAPI::Scene::remove_actor(const Actor& actor) const {
    auto& storage = registry_.get_or_emplace<Corona::Components::Storage>(scene_id_);
    auto actor_id = actor.get_id();
    std::erase(storage.actors, actor_id);

    std::uintptr_t handle = actor.get_handle_id();
    if (handle != 0) {
        bool info = Corona::SharedDataHub::instance().scene_storage().write(scene_handle_, [&](Corona::SceneDevice& slot) {
            std::erase(slot.actors, handle);
        });

        if (!info) {
            if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
                logger->warning("[CoronaEngineAPI::Scene::remove_actor] 更新场景数据失败");
            }
        }
    }
}

// ########################
//          Actor
// ########################
CoronaEngineAPI::Actor::Actor(const std::string& path)
    : animation_handle_(0), model_handle_(0), matrix_handle_(0), bounding_handle_(0), device_handle_(0) {
    registry_.emplace<RenderTag>(id_);
    auto model_id = Corona::ResourceId::from("model", path);
    auto model_ptr = std::static_pointer_cast<Corona::Model>(Corona::ResourceManager::instance().load_once(model_id));
    if (!model_ptr) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::Actor] failed to load model: " + path);
        }
        return;
    }

    auto& actor = registry_.emplace<Corona::Components::Actor>(id_);

    model_handle_ = Corona::SharedDataHub::instance().model_storage().allocate([&](std::shared_ptr<Corona::Model>& slot) {
        slot = model_ptr;
    });

    if (model_handle_ == 0) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::Actor] model_storage allocation failed for: " + path);
        }
        return;
    }

    if (model_ptr->m_BoneCounter > 0) {
        registry_.emplace<AnimationTag>(id_);
    }

    animation_handle_ = Corona::SharedDataHub::instance().animation_state_storage().allocate([&](Corona::AnimationState& slot) {
        slot.model_handle = model_handle_;
        slot.animation_index = 0;
        slot.current_time = 0.0f;
        slot.active = model_ptr->m_BoneCounter > 0;
    });

    std::vector<Corona::MeshDevice> devices;
    devices.reserve(model_ptr->meshes.size());
    for (const auto& mesh : model_ptr->meshes) {
        Corona::MeshDevice dev{};
        dev.pointsBuffer = HardwareBuffer(mesh.points, BufferUsage::VertexBuffer);
        dev.normalsBuffer = HardwareBuffer(mesh.normals, BufferUsage::VertexBuffer);
        dev.texCoordsBuffer = HardwareBuffer(mesh.texCoords, BufferUsage::VertexBuffer);
        dev.indexBuffer = HardwareBuffer(mesh.Indices, BufferUsage::IndexBuffer);
        dev.boneIndexesBuffer = HardwareBuffer(mesh.boneIndices, BufferUsage::VertexBuffer);
        dev.boneWeightsBuffer = HardwareBuffer(mesh.boneWeights, BufferUsage::VertexBuffer);
        dev.materialIndex = 0;

        if (!mesh.textures.empty() && mesh.textures[0]) {
            //dev.textureIndex = HardwareImage(mesh.textures[0]->width, mesh.textures[0]->height, ImageFormat::RGBA8_SRGB, ImageUsage::SampledImage, 1, mesh.textures[0]->data);
        } else {
            dev.textureIndex = 0;
        }

        dev.meshData = mesh;
        devices.emplace_back(std::move(dev));
    }

    matrix_handle_ = Corona::SharedDataHub::instance().model_transform_storage().allocate([&](Corona::ModelTransform& slot) {
        ktm::faffine3d affine;
        affine.translate(actor.position).rotate(actor.rotation).scale(actor.scale);
        affine >> slot.model_matrix;
    });

    bounding_handle_ = Corona::SharedDataHub::instance().model_bounding_storage().allocate([&](Corona::ModelBounding& slot) {
        slot.transform_handle = matrix_handle_;
        slot.max_xyz = model_ptr->maxXYZ;
        slot.min_xyz = model_ptr->minXYZ;
    });

    if (bounding_handle_ == 0) {
        Corona::SharedDataHub::instance().model_bounding_storage().deallocate(bounding_handle_);
        bounding_handle_ = 0;
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Actor::Actor] model_bounding_storage allocation failed for: " + path);
        }
        return;
    }

    device_handle_ = Corona::SharedDataHub::instance().model_device_storage().allocate([&](Corona::ModelDevice& slot) {
        slot.model_handle = model_handle_;  // 新增: 直接存储模型句柄用于骨骼矩阵匹配
        slot.transform_handle = matrix_handle_;
        slot.animation_handle = animation_handle_;

        if (model_ptr->m_BoneCounter > 0) {
            std::vector<ktm::fmat4x4> initial_bone_matrices(model_ptr->m_BoneCounter, ktm::fmat4x4::from_eye());
            slot.bone_matrix_buffer = HardwareBuffer(initial_bone_matrices, BufferUsage::StorageBuffer);
        } else {
            std::vector<ktm::fmat4x4> identity_matrix = {ktm::fmat4x4::from_eye()};
            slot.bone_matrix_buffer = HardwareBuffer(identity_matrix, BufferUsage::StorageBuffer);
        }

        slot.devices = std::move(devices);
    });

    if (device_handle_ == 0) {
        Corona::SharedDataHub::instance().model_storage().deallocate(model_handle_);
        model_handle_ = 0;
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::Actor] model_device_storage allocation failed for: " + path);
        }
        return;
    }

    registry_.emplace_or_replace<Corona::Components::ModelResource>(id_, Corona::Components::ModelResource{model_handle_, model_id, device_handle_});
}

CoronaEngineAPI::Actor::~Actor() {
    if (device_handle_) {
        Corona::SharedDataHub::instance().model_device_storage().deallocate(device_handle_);
    }
    if (bounding_handle_) {
        Corona::SharedDataHub::instance().model_bounding_storage().deallocate(bounding_handle_);
    }
    if (animation_handle_) {
        Corona::SharedDataHub::instance().animation_state_storage().deallocate(animation_handle_);
    }
    if (matrix_handle_) {
        Corona::SharedDataHub::instance().model_transform_storage().deallocate(matrix_handle_);
    }
    if (model_handle_) {
        Corona::SharedDataHub::instance().model_storage().deallocate(model_handle_);
    }
    registry_.destroy(id_);
}

std::uintptr_t CoronaEngineAPI::Actor::get_handle_id() const {
    return model_handle_;
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 pos) const {
    auto& actor = registry_.get<Corona::Components::Actor>(id_);
    actor.position = pos;
    bool info = Corona::SharedDataHub::instance().model_transform_storage().write(matrix_handle_, [&](Corona::ModelTransform& slot) {
        ktm::faffine3d affine;
        affine.translate(actor.position).rotate(actor.rotation).scale(actor.scale);
        affine >> slot.model_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::move] model_device_storage error");
        }
    }
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 euler) const {
    auto& actor = registry_.get<Corona::Components::Actor>(id_);
    ktm::fquat qx = ktm::fquat::from_angle_x(euler.x);
    ktm::fquat qy = ktm::fquat::from_angle_y(euler.y);
    ktm::fquat qz = ktm::fquat::from_angle_z(euler.z);

    actor.rotation = qz * qy * qx;

    bool info = Corona::SharedDataHub::instance().model_transform_storage().write(matrix_handle_, [&](Corona::ModelTransform& slot) {
        ktm::faffine3d affine;
        affine.translate(actor.position).rotate(actor.rotation).scale(actor.scale);
        affine >> slot.model_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::rotate] model_device_storage error");
        }
    }
}

void CoronaEngineAPI::Actor::scale(ktm::fvec3 size) const {
    auto& actor = registry_.get<Corona::Components::Actor>(id_);
    actor.scale = size;
    bool info = Corona::SharedDataHub::instance().model_transform_storage().write(matrix_handle_, [&](Corona::ModelTransform& slot) {
        ktm::faffine3d affine;
        affine.translate(actor.position).rotate(actor.rotation).scale(actor.scale);
        affine >> slot.model_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::scale] model_device_storage error");
        }
    }
}

// ########################
//          Camera
//  ########################
CoronaEngineAPI::Camera::Camera()
    : handle_(0) {
    ktm::fvec3 position, forward, world_up;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 5.0f;
    forward.x = 0.0f;
    forward.y = 0.0f;
    forward.z = -1.0f;
    world_up.x = 0.0f;
    world_up.y = 1.0f;
    world_up.z = 0.0f;
    float fov = 45.0f;

    registry_.emplace<Corona::Components::Camera>(id_, Corona::Components::Camera{position, forward, world_up, fov});

    handle_ = Corona::SharedDataHub::instance().camera_storage().allocate([&](Corona::CameraDevice& slot) {
        slot.eye_position = position;
        slot.eye_dir = ktm::normalize(forward);
        slot.eye_view_matrix = ktm::look_to_lh(position, ktm::normalize(forward), world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });
}

CoronaEngineAPI::Camera::Camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov)
    : handle_(0) {
    registry_.emplace<Corona::Components::Camera>(id_, Corona::Components::Camera{position, forward, world_up, fov});

    handle_ = Corona::SharedDataHub::instance().camera_storage().allocate([&](Corona::CameraDevice& slot) {
        slot.eye_position = position;
        slot.eye_dir = ktm::normalize(forward);
        slot.eye_view_matrix = ktm::look_to_lh(position, ktm::normalize(forward), world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });
}

CoronaEngineAPI::Camera::~Camera() {
    if (handle_) {
        Corona::SharedDataHub::instance().camera_storage().deallocate(handle_);
        handle_ = 0;
    }

    registry_.destroy(id_);
}

std::uintptr_t CoronaEngineAPI::Camera::get_handle_id() const {
    return handle_;
}

void CoronaEngineAPI::Camera::set_surface(void* surface) const {
    registry_.emplace_or_replace<Corona::Components::DisplaySurface>(id_, Corona::Components::DisplaySurface{surface});
    if (auto* event_bus = Corona::Kernel::KernelContext::instance().event_bus()) {
        event_bus->publish<Corona::Events::DisplaySurfaceChangedEvent>({surface});
    }

    bool info = Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.surface = reinterpret_cast<std::uint64_t>(surface);
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Camera::set_surface] 更新相机数据失败");
        }
    }
}

void CoronaEngineAPI::Camera::set_position(const ktm::fvec3& position) const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    cam.position = position;

    bool info = Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.eye_position = cam.position;
        slot.eye_dir = ktm::normalize(cam.forward);
        slot.eye_view_matrix = ktm::look_to_lh(cam.position, ktm::normalize(cam.forward), cam.world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(cam.fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Camera::set_position] 更新相机数据失败");
        }
    }
}

void CoronaEngineAPI::Camera::set_forward(const ktm::fvec3& forward) const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    cam.forward = forward;

    bool info = Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.eye_position = cam.position;
        slot.eye_dir = ktm::normalize(cam.forward);
        slot.eye_view_matrix = ktm::look_to_lh(cam.position, ktm::normalize(cam.forward), cam.world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(cam.fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Camera::set_forward] 更新相机数据失败");
        }
    }
}

void CoronaEngineAPI::Camera::set_world_up(const ktm::fvec3& world_up) const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    cam.world_up = world_up;

    bool info = Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.eye_position = cam.position;
        slot.eye_dir = ktm::normalize(cam.forward);
        slot.eye_view_matrix = ktm::look_to_lh(cam.position, ktm::normalize(cam.forward), cam.world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(cam.fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Camera::set_world_up] 更新相机数据失败");
        }
    }
}

void CoronaEngineAPI::Camera::set_fov(float fov) const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    cam.fov = fov;

    bool info = Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.eye_position = cam.position;
        slot.eye_dir = ktm::normalize(cam.forward);
        slot.eye_view_matrix = ktm::look_to_lh(cam.position, ktm::normalize(cam.forward), cam.world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(cam.fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Camera::set_fov] 更新相机数据失败");
        }
    }
}

ktm::fvec3 CoronaEngineAPI::Camera::get_position() const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    return cam.position;
}

ktm::fvec3 CoronaEngineAPI::Camera::get_forward() const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    return cam.forward;
}

ktm::fvec3 CoronaEngineAPI::Camera::get_world_up() const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    return cam.world_up;
}

float CoronaEngineAPI::Camera::get_fov() const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    return cam.fov;
}

void CoronaEngineAPI::Camera::move(ktm::fvec3 pos) const {
    auto& cam = registry_.get<Corona::Components::Camera>(id_);
    // 手动实现向量加法（ktm 可能不支持 operator+）
    cam.position.x += pos.x;
    cam.position.y += pos.y;
    cam.position.z += pos.z;

    bool info = Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.eye_position = cam.position;
        slot.eye_dir = ktm::normalize(cam.forward);
        slot.eye_view_matrix = ktm::look_to_lh(cam.position, ktm::normalize(cam.forward), cam.world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(cam.fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->warning("[CoronaEngineAPI::Camera::move] 更新相机数据失败");
        }
    }
}

void CoronaEngineAPI::Camera::rotate(ktm::fvec3 euler) const {
}

void CoronaEngineAPI::Camera::look_at(ktm::fvec3 pos, ktm::fvec3 forward) const {
}

// ########################
//          Light
// ########################
CoronaEngineAPI::Light::Light()
    : handle_(0) {
    ktm::fvec3 pos, color, dir;
    pos.x = 0.0f;
    pos.y = 5.0f;
    pos.z = 0.0f;
    color.x = 1.0f;
    color.y = 1.0f;
    color.z = 1.0f;
    dir.x = 0.0f;
    dir.y = 0.0f;
    dir.z = 0.0f;

    registry_.emplace<Corona::Components::Light>(id_, Corona::Components::Light{pos, color, dir});
    handle_ = Corona::SharedDataHub::instance().light_storage().allocate([&](Corona::LightDevice& slot) {
        // 初始化 LightDevice 相关数据
    });
}

CoronaEngineAPI::Light::~Light() {
    if (handle_) {
        Corona::SharedDataHub::instance().light_storage().deallocate(handle_);
        handle_ = 0;
    }
    registry_.destroy(id_);
}

std::uintptr_t CoronaEngineAPI::Light::get_handle_id() const {
    return handle_;
}
