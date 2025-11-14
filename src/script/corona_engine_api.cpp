//
// Created by 25473 on 25-9-19.
//

#include <CabbageHardware.h>
#include <ResourceManager.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/kernel_context.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/script/api/corona_engine_api.h>
#include <corona/shared_data_hub.h>

#include "Model.h"

// ########################
//          Scene
// ########################
Corona::API::Scene::Scene()
    : handle_(0) {
    handle_ = SharedDataHub::instance().scene_storage().allocate([&](SceneDevice& slot) {});
}

Corona::API::Scene::~Scene() {
    if (handle_ != 0) {
        SharedDataHub::instance().scene_storage().deallocate(handle_);
        handle_ = 0;
    }
}

void Corona::API::Scene::set_environment(Environment* env) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::set_environment] Invalid scene handle");
        }
        return;
    }

    if (env == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::set_environment] Null environment pointer");
        }
        return;
    }

    environment_ = env;
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        slot.environment = env->get_handle();
    });
}

void Corona::API::Scene::remove_environment() {
    if (handle_ == 0) return;

    environment_ = nullptr;
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        slot.environment = 0;
    });
}

void Corona::API::Scene::add_actor(Actor* actor) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::add_actor] Invalid scene handle");
        }
        return;
    }

    if (actor == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::add_actor] Null actor pointer");
        }
        return;
    }

    const auto actor_handle = actor->get_handle();
    if (actors_.contains(actor_handle)) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::add_actor] Actor already exists in scene, handle: " +
                            std::to_string(actor_handle));
        }
        return;
    }

    actors_.emplace(actor_handle, actor);
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        slot.actor_handles.push_back(actor_handle);
    });
}

void Corona::API::Scene::remove_actor(const Actor* actor) {
    if (handle_ == 0) return;

    if (actor == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::remove_actor] Null actor pointer");
        }
        return;
    }

    const auto actor_handle = actor->get_handle();
    if (!actors_.contains(actor_handle)) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::remove_actor] Actor not found in scene, handle: " +
                            std::to_string(actor_handle));
        }
        return;
    }

    actors_.erase(actor_handle);
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        std::erase(slot.actor_handles, actor_handle);
    });
}

void Corona::API::Scene::clear_actors() {
    if (handle_ == 0) return;

    if (auto* logger = Kernel::KernelContext::instance().logger()) {
        logger->info("[Scene::clear_actors] Clearing " + std::to_string(actors_.size()) + " actors");
    }

    actors_.clear();
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        slot.actor_handles.clear();
    });
}

bool Corona::API::Scene::has_actor(const Actor* actor) const {
    if (actor == nullptr) return false;
    return actors_.contains(actor->get_handle());
}

void Corona::API::Scene::add_viewport(Viewport* viewport) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::add_viewport] Invalid scene handle");
        }
        return;
    }

    if (viewport == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::add_viewport] Null viewport pointer");
        }
        return;
    }

    const auto vp_handle = viewport->get_handle();
    if (viewports_.contains(vp_handle)) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::add_viewport] Viewport already exists in scene, handle: " +
                            std::to_string(vp_handle));
        }
        return;
    }

    viewports_.emplace(vp_handle, viewport);
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        slot.viewport_handles.push_back(vp_handle);
    });
}

void Corona::API::Scene::remove_viewport(const Viewport* viewport) {
    if (handle_ == 0) return;

    if (viewport == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::remove_viewport] Null viewport pointer");
        }
        return;
    }

    const auto vp_handle = viewport->get_handle();
    if (!viewports_.contains(vp_handle)) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Scene::remove_viewport] Viewport not found in scene, handle: " +
                            std::to_string(vp_handle));
        }
        return;
    }

    viewports_.erase(vp_handle);
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        std::erase(slot.viewport_handles, vp_handle);
    });
}

void Corona::API::Scene::clear_viewports() {
    if (handle_ == 0) return;

    if (auto* logger = Kernel::KernelContext::instance().logger()) {
        logger->info("[Scene::clear_viewports] Clearing " + std::to_string(viewports_.size()) + " viewports");
    }

    viewports_.clear();
    SharedDataHub::instance().scene_storage().write(handle_, [&](SceneDevice& slot) {
        slot.viewport_handles.clear();
    });
}

bool Corona::API::Scene::has_viewport(const Viewport* viewport) const {
    if (viewport == nullptr) return false;
    return viewports_.contains(viewport->get_handle());
}

// ########################
//      Environment
// ########################
Corona::API::Environment::Environment()
    : handle_(0) {
    handle_ = SharedDataHub::instance().environment_storage().allocate([&](EnvironmentDevice& slot) {});
}

Corona::API::Environment::~Environment() {
    if (handle_ != 0) {
        SharedDataHub::instance().environment_storage().deallocate(handle_);
        handle_ = 0;
    }
}

void Corona::API::Environment::set_sun_direction(const std::array<float, 3>& direction) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Environment::set_sun_direction] Invalid environment handle");
        }
        return;
    }

    SharedDataHub::instance().environment_storage().write(handle_, [&](EnvironmentDevice& slot) {
        slot.sun_position.x = direction[0];
        slot.sun_position.y = direction[1];
        slot.sun_position.z = direction[2];
    });

    if (auto* logger = Kernel::KernelContext::instance().logger()) {
        logger->info("[Environment::set_sun_direction] Sun direction set to: (" +
                     std::to_string(direction[0]) + ", " +
                     std::to_string(direction[1]) + ", " +
                     std::to_string(direction[2]) + ")");
    }
}

void Corona::API::Environment::set_floor_grid(bool enabled) const {
    // TODO: Implement floor grid rendering control
    if (auto* logger = Kernel::KernelContext::instance().logger()) {
        logger->warning("[Corona::API::Environment::set_floor_grid] Not implemented yet: " +
                        std::string(enabled ? "enabled" : "disabled"));
    }
}

// ########################
//         Geometry
// ########################
Corona::API::Geometry::Geometry(const std::string& model_path)
    : handle_(0), transform_handle_(0), model_resource_handle_(0) {
    auto model_id = ResourceId::from("model", model_path);
    auto model_ptr = std::static_pointer_cast<Model>(ResourceManager::instance().load_once(model_id));
    if (!model_ptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->error("[Corona::API::Geometry] Failed to load model: " + model_path);
        }
        return;
    }

    // 1. 分配 ModelResource
    model_resource_handle_ = SharedDataHub::instance().model_resource_storage().allocate(
        [&](ModelResource& slot) {
            slot.model_ptr = model_ptr;
        });

    // 2. 分配 ModelTransform（使用局部参数，默认值已在结构体中定义）
    transform_handle_ = SharedDataHub::instance().model_transform_storage().allocate(
        [&](ModelTransform& slot) {
            // 默认值：position(0,0,0), rotation(0,0,0), scale(1,1,1)
            // 已在 ModelTransform 结构体中初始化
        });

    // 3. 创建 MeshDevice 列表
    std::vector<MeshDevice> mesh_devices;
    mesh_devices.reserve(model_ptr->meshes.size());
    for (const auto& mesh : model_ptr->meshes) {
        MeshDevice dev{};
        dev.pointsBuffer = HardwareBuffer(mesh.points, BufferUsage::VertexBuffer);
        dev.normalsBuffer = HardwareBuffer(mesh.normals, BufferUsage::VertexBuffer);
        dev.texCoordsBuffer = HardwareBuffer(mesh.texCoords, BufferUsage::VertexBuffer);
        dev.indexBuffer = HardwareBuffer(mesh.Indices, BufferUsage::IndexBuffer);
        dev.boneIndexesBuffer = HardwareBuffer(mesh.boneIndices, BufferUsage::VertexBuffer);
        dev.boneWeightsBuffer = HardwareBuffer(mesh.boneWeights, BufferUsage::VertexBuffer);
        dev.materialIndex = 0;

        if (!mesh.textures.empty() && mesh.textures[0]) {
            dev.textureIndex = HardwareImage(mesh.textures[0]->width, mesh.textures[0]->height,
                                             ImageFormat::RGBA8_SRGB, ImageUsage::SampledImage, 1,
                                             mesh.textures[0]->data);
        } else {
            dev.textureIndex = 0;
        }

        dev.meshData = mesh;
        mesh_devices.emplace_back(std::move(dev));
    }

    // 4. 分配 GeometryDevice
    handle_ = SharedDataHub::instance().geometry_storage().allocate(
        [&](GeometryDevice& slot) {
            slot.transform_handle = transform_handle_;
            slot.model_resource_handle = model_resource_handle_;
            slot.mesh_handles = std::move(mesh_devices);
        });

    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->error("[Corona::API::Geometry] Failed to allocate GeometryDevice");
        }
    }
}

Corona::API::Geometry::~Geometry() {
    if (handle_ != 0) {
        SharedDataHub::instance().geometry_storage().deallocate(handle_);
    }
    if (transform_handle_ != 0) {
        SharedDataHub::instance().model_transform_storage().deallocate(transform_handle_);
    }
    if (model_resource_handle_ != 0) {
        SharedDataHub::instance().model_resource_storage().deallocate(model_resource_handle_);
    }
}

void Corona::API::Geometry::set_position(const std::array<float, 3>& pos) {
    if (transform_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Geometry::set_position] Invalid transform handle");
        }
        return;
    }

    // 直接写入容器中的局部位置参数
    SharedDataHub::instance().model_transform_storage().write(transform_handle_, [&](ModelTransform& slot) {
        slot.position.x = pos[0];
        slot.position.y = pos[1];
        slot.position.z = pos[2];
    });
}

void Corona::API::Geometry::set_rotation(const std::array<float, 3>& euler) {
    if (transform_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Geometry::set_rotation] Invalid transform handle");
        }
        return;
    }

    // 直接写入容器中的局部旋转参数（欧拉角 ZYX 顺序）
    SharedDataHub::instance().model_transform_storage().write(transform_handle_, [&](ModelTransform& slot) {
        slot.euler_rotation.x = euler[0];  // Pitch
        slot.euler_rotation.y = euler[1];  // Yaw
        slot.euler_rotation.z = euler[2];  // Roll
    });
}

void Corona::API::Geometry::set_scale(const std::array<float, 3>& scl) {
    if (transform_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Geometry::set_scale] Invalid transform handle");
        }
        return;
    }

    // 直接写入容器中的局部缩放参数
    SharedDataHub::instance().model_transform_storage().write(transform_handle_, [&](ModelTransform& slot) {
        slot.scale.x = scl[0];
        slot.scale.y = scl[1];
        slot.scale.z = scl[2];
    });
}

std::array<float, 3> Corona::API::Geometry::get_position() const {
    if (transform_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Geometry::get_position] Invalid transform handle");
        }
        return {0.0f, 0.0f, 0.0f};
    }

    // 从容器中读取局部位置参数
    std::array<float, 3> result = {0.0f, 0.0f, 0.0f};
    SharedDataHub::instance().model_transform_storage().read(transform_handle_, [&](const ModelTransform& slot) {
        result[0] = slot.position.x;
        result[1] = slot.position.y;
        result[2] = slot.position.z;
    });

    return result;
}

std::array<float, 3> Corona::API::Geometry::get_rotation() const {
    if (transform_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Geometry::get_rotation] Invalid transform handle");
        }
        return {0.0f, 0.0f, 0.0f};
    }

    // 从容器中读取局部旋转参数（欧拉角 ZYX 顺序）
    std::array<float, 3> result = {0.0f, 0.0f, 0.0f};
    SharedDataHub::instance().model_transform_storage().read(transform_handle_, [&](const ModelTransform& slot) {
        result[0] = slot.euler_rotation.x;  // Pitch
        result[1] = slot.euler_rotation.y;  // Yaw
        result[2] = slot.euler_rotation.z;  // Roll
    });

    return result;
}

std::array<float, 3> Corona::API::Geometry::get_scale() const {
    if (transform_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Geometry::get_scale] Invalid transform handle");
        }
        return {1.0f, 1.0f, 1.0f};
    }

    // 从容器中读取局部缩放参数
    std::array<float, 3> result = {1.0f, 1.0f, 1.0f};
    SharedDataHub::instance().model_transform_storage().read(transform_handle_, [&](const ModelTransform& slot) {
        result[0] = slot.scale.x;
        result[1] = slot.scale.y;
        result[2] = slot.scale.z;
    });

    return result;
}

// ########################
//         Optics
// ########################
Corona::API::Optics::Optics(Geometry& geo)
    : geometry_(&geo), handle_(0), skinning_handle_(0) {
    bool has_bones = false;
    SharedDataHub::instance().geometry_storage().read(geo.get_handle(), [&](const GeometryDevice& geom_dev) {
        SharedDataHub::instance().model_resource_storage().read(geom_dev.model_resource_handle, [&](const ModelResource& res) {
            has_bones = (res.model_ptr && res.model_ptr->m_BoneCounter > 0);
        });
    });

    if (has_bones) {
        size_t bone_count = 0;
        SharedDataHub::instance().geometry_storage().read(geo.get_handle(), [&](const GeometryDevice& geom_dev) {
            SharedDataHub::instance().model_resource_storage().read(geom_dev.model_resource_handle, [&](const ModelResource& res) {
                bone_count = res.model_ptr->m_BoneCounter;
            });
        });

        skinning_handle_ = SharedDataHub::instance().skinning_storage().allocate([&](SkinningDevice& slot) {
            std::vector<ktm::fmat4x4> initial_matrices(bone_count, ktm::fmat4x4::from_eye());
            slot.bone_matrix_buffer = HardwareBuffer(initial_matrices, BufferUsage::StorageBuffer);
        });
    }

    handle_ = SharedDataHub::instance().optics_storage().allocate([&](OpticsDevice& slot) {
        slot.geometry_handle = geo.get_handle();
        slot.skinning_handle = skinning_handle_;
    });
}

Corona::API::Optics::~Optics() {
    if (handle_ != 0) {
        SharedDataHub::instance().optics_storage().deallocate(handle_);
    }
    if (skinning_handle_ != 0) {
        SharedDataHub::instance().skinning_storage().deallocate(skinning_handle_);
    }
}

// ########################
//       Mechanics
// ########################
Corona::API::Mechanics::Mechanics(Geometry& geo)
    : geometry_(&geo), handle_(0) {
    // 获取模型的包围盒信息
    ktm::fvec3 max_xyz{0, 0, 0};
    ktm::fvec3 min_xyz{0, 0, 0};
    SharedDataHub::instance().geometry_storage().read(geo.get_handle(), [&](const GeometryDevice& geom_dev) {
        SharedDataHub::instance().model_resource_storage().read(geom_dev.model_resource_handle, [&](const ModelResource& res) {
            if (res.model_ptr) {
                max_xyz = res.model_ptr->maxXYZ;
                min_xyz = res.model_ptr->minXYZ;
            }
        });
    });

    // 创建 MechanicsDevice
    handle_ = SharedDataHub::instance().mechanics_storage().allocate([&](MechanicsDevice& slot) {
        slot.geometry_handle = geo.get_handle();
        slot.max_xyz = max_xyz;
        slot.min_xyz = min_xyz;
    });
}

Corona::API::Mechanics::~Mechanics() {
    if (handle_ != 0) {
        SharedDataHub::instance().mechanics_storage().deallocate(handle_);
    }
}

// ########################
//       Acoustics
// ########################
Corona::API::Acoustics::Acoustics(Geometry& geo)
    : geometry_(&geo), handle_(0), volume_(1.0f) {
    handle_ = SharedDataHub::instance().acoustics_storage().allocate([&](AcousticsDevice& slot) {
        slot.geometry_handle = geo.get_handle();
    });
}

Corona::API::Acoustics::~Acoustics() {
    if (handle_ != 0) {
        SharedDataHub::instance().acoustics_storage().deallocate(handle_);
    }
}

// ########################
//       Kinematics
// ########################
Corona::API::Kinematics::Kinematics(Geometry& geo)
    : geometry_(&geo), handle_(0), animation_handle_(0), skinning_handle_(0) {
    // 检查是否有骨骼动画
    bool has_bones = false;
    size_t bone_count = 0;
    SharedDataHub::instance().geometry_storage().read(geo.get_handle(), [&](const GeometryDevice& geom_dev) {
        SharedDataHub::instance().model_resource_storage().read(geom_dev.model_resource_handle, [&](const ModelResource& res) {
            if (res.model_ptr) {
                has_bones = (res.model_ptr->m_BoneCounter > 0);
                bone_count = res.model_ptr->m_BoneCounter;
            }
        });
    });

    // 创建 AnimationController
    animation_handle_ = SharedDataHub::instance().animation_controller_storage().allocate([&](AnimationState& slot) {
        slot.animation_index = 0;
        slot.current_time = 0.0f;
        slot.playback_speed = 1.0f;
        slot.active = has_bones;
    });

    // 如果有骨骼，创建 SkinningDevice
    if (has_bones) {
        skinning_handle_ = SharedDataHub::instance().skinning_storage().allocate([&](SkinningDevice& slot) {
            std::vector<ktm::fmat4x4> initial_matrices(bone_count, ktm::fmat4x4::from_eye());
            slot.bone_matrix_buffer = HardwareBuffer(initial_matrices, BufferUsage::StorageBuffer);
        });
    }

    // 创建 KinematicsDevice
    handle_ = SharedDataHub::instance().kinematics_storage().allocate([&](KinematicsDevice& slot) {
        slot.geometry_handle = geo.get_handle();
        slot.skinning_handle = skinning_handle_;
        slot.animation_controller_handle = animation_handle_;
    });
}

Corona::API::Kinematics::~Kinematics() {
    if (handle_ != 0) {
        SharedDataHub::instance().kinematics_storage().deallocate(handle_);
    }
    if (animation_handle_ != 0) {
        SharedDataHub::instance().animation_controller_storage().deallocate(animation_handle_);
    }
    if (skinning_handle_ != 0) {
        SharedDataHub::instance().skinning_storage().deallocate(skinning_handle_);
    }
}

void Corona::API::Kinematics::set_animation(std::uint32_t animation_index) {
    if (handle_ == 0 || animation_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Kinematics::set_animation] Invalid handle");
        }
        return;
    }

    animation_index_ = animation_index;
    SharedDataHub::instance().animation_controller_storage().write(animation_handle_, [&](AnimationState& slot) {
        slot.animation_index = animation_index;
        slot.current_time = 0.0f;  // 重置时间
    });

    // 读回验证
    SharedDataHub::instance().animation_controller_storage().read(animation_handle_, [&](const AnimationState& slot) {
        if (slot.animation_index != animation_index) {
            if (auto* logger = Kernel::KernelContext::instance().logger()) {
                logger->warning("[Kinematics::set_animation] State sync mismatch");
            }
        }
    });
}

void Corona::API::Kinematics::play_animation(float speed) {
    if (handle_ == 0 || animation_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Kinematics::play_animation] Invalid handle");
        }
        return;
    }

    speed_ = speed;
    active_ = true;

    SharedDataHub::instance().animation_controller_storage().write(animation_handle_, [&](AnimationState& slot) {
        slot.playback_speed = speed;
        slot.active = true;
    });
}

void Corona::API::Kinematics::stop_animation() {
    if (handle_ == 0 || animation_handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Kinematics::stop_animation] Invalid handle");
        }
        return;
    }

    active_ = false;

    SharedDataHub::instance().animation_controller_storage().write(animation_handle_, [&](AnimationState& slot) {
        slot.active = false;
    });
}

// ########################
//          Actor
// ########################
Corona::API::Actor::Actor()
    : handle_(0), active_profile_handle_(0), next_profile_handle_(1) {
    handle_ = SharedDataHub::instance().actor_storage().allocate([&](ActorDevice& slot) {
    });
}

Corona::API::Actor::~Actor() {
    if (handle_ != 0) {
        SharedDataHub::instance().actor_storage().deallocate(handle_);
    }
}

Corona::API::Actor::Profile* Corona::API::Actor::add_profile(Profile profile) {
    std::uintptr_t profile_handle = next_profile_handle_++;
    profiles_[profile_handle] = profile;

    // 如果是第一个 profile，自动设为激活
    if (active_profile_handle_ == 0) {
        active_profile_handle_ = profile_handle;
    }

    // 更新 ActorDevice 中的 profile_handles
    if (handle_ != 0) {
        SharedDataHub::instance().actor_storage().write(handle_, [&](ActorDevice& slot) {
            slot.profile_handles.push_back(profile_handle);
        });
    }

    return &profiles_[profile_handle];
}

void Corona::API::Actor::remove_profile(Profile* profile) {
    if (handle_ == 0 || profile == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Actor::remove_profile] Invalid actor handle or null profile");
        }
        return;
    }

    // 查找该 Profile 指针对应的 handle
    std::uintptr_t profile_handle = 0;
    for (const auto& [handle, prof] : profiles_) {
        if (&prof == profile) {
            profile_handle = handle;
            break;
        }
    }

    if (profile_handle == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Actor::remove_profile] Profile not found in this actor");
        }
        return;
    }

    auto it = profiles_.find(profile_handle);
    if (it == profiles_.end()) {
        return;
    }

    // 停用组件（如动画）
    if (it->second.kinematics) {
        it->second.kinematics->stop_animation();
    }

    // 注意：组件指针由外部管理，这里只移除引用
    // 不销毁实际的组件对象

    // 从 map 中删除
    profiles_.erase(it);

    // 如果删除的是激活的 profile，选择新的激活项
    if (active_profile_handle_ == profile_handle) {
        if (!profiles_.empty()) {
            active_profile_handle_ = profiles_.begin()->first;
            if (auto* logger = Kernel::KernelContext::instance().logger()) {
                logger->info("[Actor::remove_profile] Switched to first available profile");
            }
        } else {
            active_profile_handle_ = 0;
            if (auto* logger = Kernel::KernelContext::instance().logger()) {
                logger->info("[Actor::remove_profile] No profiles remaining");
            }
        }
    }

    // 更新 ActorDevice
    SharedDataHub::instance().actor_storage().write(handle_, [&](ActorDevice& slot) {
        std::erase(slot.profile_handles, profile_handle);
    });
}

void Corona::API::Actor::set_active_profile(Profile* profile) {
    if (profile == nullptr) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Actor::set_active_profile] Null profile pointer");
        }
        return;
    }

    // 查找该 Profile 指针对应的 handle
    for (const auto& [handle, prof] : profiles_) {
        if (&prof == profile) {
            active_profile_handle_ = handle;
            return;
        }
    }

    if (auto* logger = Kernel::KernelContext::instance().logger()) {
        logger->warning("[Actor::set_active_profile] Profile not found in this actor");
    }
}

Corona::API::Actor::Profile* Corona::API::Actor::get_active_profile() {
    if (active_profile_handle_ == 0) return nullptr;
    auto it = profiles_.find(active_profile_handle_);
    return (it != profiles_.end()) ? &it->second : nullptr;
}

// ########################
//          Camera
// ########################
Corona::API::Camera::Camera()
    : handle_(0) {
    ktm::fvec3 pos_vec, fwd_vec, up_vec;
    pos_vec.x = 0.0f;
    pos_vec.y = 0.0f;
    pos_vec.z = 5.0f;
    fwd_vec.x = 0.0f;
    fwd_vec.y = 0.0f;
    fwd_vec.z = -1.0f;
    up_vec.x = 0.0f;
    up_vec.y = 1.0f;
    up_vec.z = 0.0f;
    float fov = 45.0f;

    handle_ = SharedDataHub::instance().camera_storage().allocate([&](CameraDevice& slot) {
        slot.position = pos_vec;
        slot.forward = fwd_vec;
        slot.world_up = up_vec;
        slot.fov = fov;
    });
}

Corona::API::Camera::Camera(const std::array<float, 3>& position, const std::array<float, 3>& forward, const std::array<float, 3>& world_up, float fov)
    : handle_(0) {
    ktm::fvec3 pos_vec, fwd_vec, up_vec;
    pos_vec.x = position[0];
    pos_vec.y = position[1];
    pos_vec.z = position[2];
    fwd_vec.x = forward[0];
    fwd_vec.y = forward[1];
    fwd_vec.z = forward[2];
    up_vec.x = world_up[0];
    up_vec.y = world_up[1];
    up_vec.z = world_up[2];
    handle_ = SharedDataHub::instance().camera_storage().allocate([&](CameraDevice& slot) {
        slot.position = pos_vec;
        slot.forward = fwd_vec;
        slot.world_up = up_vec;
        slot.fov = fov;
    });
}

Corona::API::Camera::~Camera() {
    if (handle_) {
        SharedDataHub::instance().camera_storage().deallocate(handle_);
        handle_ = 0;
    }
}

void Corona::API::Camera::set(const std::array<float, 3>& position, const std::array<float, 3>& forward, const std::array<float, 3>& world_up, float fov) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Camera::set] Invalid camera handle");
        }
        return;
    }

    ktm::fvec3 pos_vec, fwd_vec, up_vec;
    pos_vec.x = position[0];
    pos_vec.y = position[1];
    pos_vec.z = position[2];
    fwd_vec.x = forward[0];
    fwd_vec.y = forward[1];
    fwd_vec.z = forward[2];
    up_vec.x = world_up[0];
    up_vec.y = world_up[1];
    up_vec.z = world_up[2];

    SharedDataHub::instance().camera_storage().write(handle_, [&](CameraDevice& slot) {
        slot.position = pos_vec;
        slot.forward = fwd_vec;
        slot.world_up = up_vec;
        slot.fov = fov;
    });
}

std::array<float, 3> Corona::API::Camera::get_position() const {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Camera::get_position] Invalid camera handle");
        }
        return {0.0f, 0.0f, 0.0f};
    }

    // 从 storage 同步读取最新状态
    std::array result = {0.0f, 0.0f, 0.0f};
    SharedDataHub::instance().camera_storage().read(handle_, [&](const CameraDevice& slot) {
        result[0] = slot.position.x;
        result[1] = slot.position.y;
        result[2] = slot.position.z;
    });

    return result;
}

std::array<float, 3> Corona::API::Camera::get_forward() const {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Camera::get_forward] Invalid camera handle");
        }
        return {0.0f, 0.0f, -1.0f};
    }

    std::array result = {0.0f, 0.0f, -1.0f};
    SharedDataHub::instance().camera_storage().read(handle_, [&](const CameraDevice& slot) {
        result[0] = slot.forward.x;
        result[1] = slot.forward.y;
        result[2] = slot.forward.z;
    });

    return result;
}

std::array<float, 3> Corona::API::Camera::get_world_up() const {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Camera::get_world_up] Invalid camera handle");
        }
        return {0.0f, 1.0f, 0.0f};
    }

    std::array result = {0.0f, 1.0f, 0.0f};
    SharedDataHub::instance().camera_storage().read(handle_, [&](const CameraDevice& slot) {
        result[0] = slot.world_up.x;
        result[1] = slot.world_up.y;
        result[2] = slot.world_up.z;
    });

    return result;
}

float Corona::API::Camera::get_fov() const {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Camera::get_fov] Invalid camera handle");
        }
        return 45.0f;
    }

    float result = 45.0f;
    SharedDataHub::instance().camera_storage().read(handle_, [&](const CameraDevice& slot) {
        result = slot.fov;
    });

    return result;
}

void Corona::API::Camera::set_surface(void* surface) {
    if (surface) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Camera::set_surface] Invalid camera handle");
        }
        return;
    }
    SharedDataHub::instance().camera_storage().write(handle_, [&](CameraDevice& slot) {
        slot.surface = surface;
    });

    if (auto* event_bus = Kernel::KernelContext::instance().event_bus()) {
        event_bus->publish<Events::DisplaySurfaceChangedEvent>({surface});
    }
}

// ########################
//      ImageEffects
// ########################
Corona::API::ImageEffects::ImageEffects()
    : handle_(0) {
}

Corona::API::ImageEffects::~ImageEffects() {
    if (handle_ != 0) {
        // Corona::SharedDataHub::instance().image_effects_storage().deallocate(handle_);
        handle_ = 0;
    }
}

// ########################
//        Viewport
// ########################
Corona::API::Viewport::Viewport()
    : handle_(0) {
    handle_ = SharedDataHub::instance().viewport_storage().allocate([&](ViewportDevice& slot) {
        slot.camera = 0;  // 初始无 Camera
    });
}

Corona::API::Viewport::Viewport(int width, int height, bool light_field)
    : handle_(0), width_(width), height_(height) {
    handle_ = SharedDataHub::instance().viewport_storage().allocate([&](ViewportDevice& slot) {
        slot.camera = 0;  // 初始无 Camera
    });
}

Corona::API::Viewport::~Viewport() {
    if (handle_ != 0) {
        SharedDataHub::instance().viewport_storage().deallocate(handle_);
        handle_ = 0;
    }
}

void Corona::API::Viewport::set_camera(Camera* camera) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Viewport::set_camera] Invalid viewport handle");
        }
        return;
    }

    camera_ = camera;  // 仅存储指针，不拥有所有权

    // 更新 viewport_storage 中的 camera handle
    SharedDataHub::instance().viewport_storage().write(handle_, [&](ViewportDevice& slot) {
        slot.camera = camera_ ? camera_->get_handle() : 0;
    });
}

void Corona::API::Viewport::remove_camera() {
    if (handle_ == 0) return;

    camera_ = nullptr;  // 仅移除引用，不销毁对象
    SharedDataHub::instance().viewport_storage().write(handle_, [&](ViewportDevice& slot) {
        slot.camera = 0;
    });
}

void Corona::API::Viewport::set_image_effects(ImageEffects* effects) {
    image_effects_ = effects;  // 仅存储指针，不拥有所有权
    // TODO: 如果有 image_effects_storage，在此写入
}

void Corona::API::Viewport::remove_image_effects() {
    image_effects_ = nullptr;  // 仅移除引用，不销毁对象
    // TODO: 如果有 image_effects_storage，在此清理
}

void Corona::API::Viewport::set_size(int width, int height) {
    if (handle_ == 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Viewport::set_size] Invalid viewport handle");
        }
        return;
    }

    if (width <= 0 || height <= 0) {
        if (auto* logger = Kernel::KernelContext::instance().logger()) {
            logger->warning("[Viewport::set_size] Invalid size: " +
                            std::to_string(width) + "x" + std::to_string(height));
        }
        return;
    }

    width_ = width;
    height_ = height;

    SharedDataHub::instance().viewport_storage().write(handle_, [&](ViewportDevice& slot) {

    });
}

void Corona::API::Viewport::set_viewport_rect(int x, int y, int width, int height) {
    // TODO: Implement viewport rectangle settings
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[Corona::API::Viewport::set_viewport_rect] Not implemented yet");
    }
}

void Corona::API::Viewport::set_surface(void* surface) {
    surface_ = surface;
    if (auto* event_bus = Kernel::KernelContext::instance().event_bus()) {
        event_bus->publish<Events::DisplaySurfaceChangedEvent>({surface});
    }
}

void Corona::API::Viewport::pick_actor_at_pixel(int x, int y) const {
    // TODO: Implement pixel picking for actor selection
    if (auto* logger = Kernel::KernelContext::instance().logger()) {
        logger->warning("[Corona::API::Viewport::pick_actor_at_pixel] Not implemented yet");
    }
}

void Corona::API::Viewport::save_screenshot(const std::string& path) const {
    // TODO: Implement screenshot functionality
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[Corona::API::Viewport::save_screenshot] Not implemented yet: " + path);
    }
}
