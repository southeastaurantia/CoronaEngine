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
    :handle_(0){
    handle_ = SharedDataHub::instance().scene_storage().allocate([&](const SceneDevice& slot) {
        slot.scene_handle = handle_;
    });
}
Corona::API::Scene::Scene(bool light_field)
    :handle_(0){
}
Corona::API::Scene::~Scene() {
    if (handle_ != 0) {
        Corona::SharedDataHub::instance().scene_storage().deallocate(handle_);
        handle_ = 0;
    }
}
void Corona::API::Scene::set_environment(Environment* env) {

}
void Corona::API::Scene::remove_environment() {
}
void Corona::API::Scene::add_actor(Actor* actor) {
}
void Corona::API::Scene::remove_actor(Actor* actor) {
}
void Corona::API::Scene::remove_actor_at(std::size_t index) {
}
void Corona::API::Scene::clear_actors() {
}
Corona::API::Actor* Corona::API::Scene::get_actor(std::size_t index) {
}
const Corona::API::Actor* Corona::API::Scene::get_actor(std::size_t index) const {
}
bool Corona::API::Scene::has_actor(const Actor* actor) const {
}
void Corona::API::Scene::add_viewport(Viewport* viewport) {
}
void Corona::API::Scene::remove_viewport(Viewport* viewport) {
}
void Corona::API::Scene::remove_viewport_at(std::size_t index) {
}
void Corona::API::Scene::clear_viewports() {
}
Corona::API::Viewport* Corona::API::Scene::get_viewport(std::size_t index) {
}
const Corona::API::Viewport* Corona::API::Scene::get_viewport(std::size_t index) const {
}
bool Corona::API::Scene::has_viewport(const Viewport* viewport) const {
}


// ########################
//        Geometry
// ########################
// Internal data storage for Geometry (without ECS)
struct GeometryData {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
    std::array<std::array<float,4>, 4> rotation = {{{1.0f, 0.0f, 0.0f, 0.0f},;
                                              {0.0f, 1.0f, 0.0f, 0.0f},
                                              {0.0f, 0.0f, 1.0f, 0.0f},
                                              {0.0f, 0.0f, 0.0f, 1.0f}}};
};

// Static storage map for geometry data
static std::unordered_map<const CoronaEngineAPI::Geometry*, GeometryData> geometry_data_map;

CoronaEngineAPI::Geometry::Geometry() {
    geometry_data_map[this] = GeometryData{};
}

CoronaEngineAPI::Geometry::~Geometry() {
    geometry_data_map.erase(this);
}

void CoronaEngineAPI::Geometry::set_position(const std::array<float, 3>& position) {
    auto& data = geometry_data_map[this];
    data.position[0] = position[0];
    data.position[1] = position[1];
    data.position[2] = position[2];
}

void CoronaEngineAPI::Geometry::set_rotation(const std::array<float, 3>& euler) {
    auto& data = geometry_data_map[this];
    ktm::fquat qx = ktm::fquat::from_angle_x(euler[0]);
    ktm::fquat qy = ktm::fquat::from_angle_y(euler[1]);
    ktm::fquat qz = ktm::fquat::from_angle_z(euler[2]);
    data.rotation = qz * qy * qx;
}

void CoronaEngineAPI::Geometry::set_scale(const std::array<float, 3>& size) {
    auto& data = geometry_data_map[this];
    data.scale[0] = size[0];
    data.scale[1] = size[1];
    data.scale[2] = size[2];
}

std::array<float, 3> CoronaEngineAPI::Geometry::get_position() const {
    auto& data = geometry_data_map[this];
    return {data.position[0], data.position[1], data.position[2]};
}

std::array<float, 3> CoronaEngineAPI::Geometry::get_rotation() const {
    // TODO: Implement proper quaternion to euler conversion for ktm library
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Geometry::get_rotation] Quaternion to euler conversion not fully implemented");
    }
    return {0.0f, 0.0f, 0.0f};
}

std::array<float, 3> CoronaEngineAPI::Geometry::get_scale() const {
    auto& data = geometry_data_map[this];
    return {data.scale[0], data.scale[1], data.scale[2]};
}

// ########################
//        Mechanics
// ########################
CoronaEngineAPI::Mechanics::Mechanics() {
}

CoronaEngineAPI::Mechanics::~Mechanics() {
}


// ########################
//        Optics
// ########################
CoronaEngineAPI::Optics::Optics() {
    // Optics component for rendering/lighting properties
}

CoronaEngineAPI::Optics::~Optics() {
}

// ########################
//        Acoustics
// ########################
CoronaEngineAPI::Acoustics::Acoustics() {
    // Acoustics component for audio properties
}

CoronaEngineAPI::Acoustics::~Acoustics() {
}

// ########################
//        Kinematics
// ########################
CoronaEngineAPI::Kinematics::Kinematics() {
}

CoronaEngineAPI::Kinematics::~Kinematics() {
}

void CoronaEngineAPI::Kinematics::set_skeletal_anim() {
    // TODO: Implement skeletal animation control
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Kinematics::set_skeletal_anim] Not implemented yet");
    }
}

// ########################
//          Actor
// ########################
CoronaEngineAPI::Actor::Actor(const std::string& path)
    {
    auto model_id = Corona::ResourceId::from("model", path);
    auto model_ptr = std::static_pointer_cast<Corona::Model>(Corona::ResourceManager::instance().load_once(model_id));
    if (!model_ptr) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::Actor] failed to load model: " + path);
        }
        return;
    }

    auto& actor = registry_.emplace<Corona::Components::Geometry>(id_);

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
        registry_.emplace<KinematicsTag>(id_);
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
            dev.textureIndex = HardwareImage(mesh.textures[0]->width, mesh.textures[0]->height, ImageFormat::RGBA8_SRGB, ImageUsage::SampledImage, 1, mesh.textures[0]->data);
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
}

std::uintptr_t CoronaEngineAPI::Actor::get_handle_id() const {
    return model_handle_;
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

    handle_ = Corona::SharedDataHub::instance().camera_storage().allocate([&](Corona::CameraDevice& slot) {
        slot.eye_position = position;
        slot.eye_dir = ktm::normalize(forward);
        slot.eye_view_matrix = ktm::look_at_lh(position, ktm::normalize(forward), world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });
}
CoronaEngineAPI::Camera::Camera(const std::array<float, 3>& position, const std::array<float, 3>& forward, const std::array<float, 3>& world_up, float fov)
    : handle_(0) {

    handle_ = Corona::SharedDataHub::instance().camera_storage().allocate([&](Corona::CameraDevice& slot) {
        slot.eye_position = position;
        slot.eye_dir = ktm::normalize(forward);
        slot.eye_view_matrix = ktm::look_at_lh(position, ktm::normalize(forward), world_up);
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });
}

CoronaEngineAPI::Camera::~Camera() {
    if (handle_) {
        Corona::SharedDataHub::instance().camera_storage().deallocate(handle_);
        handle_ = 0;
    }
}

std::uintptr_t CoronaEngineAPI::Camera::get_handle_id() const {
    return handle_;
}

void CoronaEngineAPI::Camera::set(std::array<float, 3>& position, std::array<float, 3>& forward, std::array<float, 3> world_up, float fov) const {

    Corona::SharedDataHub::instance().camera_storage().write(handle_, [&](Corona::CameraDevice& slot) {
        slot.eye_position = ktm::fvec3{position[0], position[1], position[2]};
        slot.eye_dir = ktm::normalize(ktm::fvec3{forward[0], forward[1], forward[2]});
        slot.eye_view_matrix = ktm::look_at_lh(slot.eye_position, slot.eye_position + slot.eye_dir, ktm::fvec3{world_up[0], world_up[1], world_up[2]});
        slot.eye_proj_matrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
        slot.view_proj_matrix = slot.eye_proj_matrix * slot.eye_view_matrix;
    });
}

std::array<float, 3> CoronaEngineAPI::Camera::get_position() const {
}

std::array<float, 3> CoronaEngineAPI::Camera::get_forward() const {

}

std::array<float, 3> CoronaEngineAPI::Camera::get_world_up() const {

}

float CoronaEngineAPI::Camera::get_fov() const {

}

// ########################
//      ImageEffects
// ########################
CoronaEngineAPI::ImageEffects::ImageEffects() {
}

CoronaEngineAPI::ImageEffects::~ImageEffects() {
}

// ########################
//        Viewport
// ########################
CoronaEngineAPI::Viewport::Viewport() {
}

CoronaEngineAPI::Viewport::~Viewport() {
}

void CoronaEngineAPI::Viewport::bind_camera() {
    // TODO: Bind camera to viewport for rendering
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Viewport::bind_camera] Not implemented yet");
    }
}

void CoronaEngineAPI::Viewport::unbind_camera() {
    // TODO: Unbind camera from viewport
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Viewport::unbind_camera] Not implemented yet");
    }
}

void CoronaEngineAPI::Viewport::bind_image_effects() {
    // TODO: Bind post-processing effects to viewport
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Viewport::bind_image_effects] Not implemented yet");
    }
}

void CoronaEngineAPI::Viewport::unbind_image_effects() {
    // TODO: Unbind post-processing effects from viewport
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Viewport::unbind_image_effects] Not implemented yet");
    }
}

void CoronaEngineAPI::Viewport::detect_actor_by_pixel() {
    // TODO: Implement pixel picking for actor selection
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Viewport::detect_actor_by_pixel] Not implemented yet");
    }
}

void CoronaEngineAPI::Viewport::set_surface(void* surface) const {
    if (auto* event_bus = Corona::Kernel::KernelContext::instance().event_bus()) {
        event_bus->publish<Corona::Events::DisplaySurfaceChangedEvent>({surface});
    }
}

void CoronaEngineAPI::Viewport::save_picture(const std::string& path) const {
    // TODO: Implement screenshot functionality
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Viewport::save_picture] Not implemented yet: " + path);
    }
}

// ########################
//      Environment
// ########################
CoronaEngineAPI::Environment::Environment() {
}

CoronaEngineAPI::Environment::~Environment() {
}

void CoronaEngineAPI::Environment::set_sun_direction(const ktm::fvec3& direction) const {
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->info("[CoronaEngineAPI::Environment::set_sun_direction] Sun direction set to: (" +
                     std::to_string(direction.x) + ", " +
                     std::to_string(direction.y) + ", " +
                     std::to_string(direction.z) + ")");
    }
}

void CoronaEngineAPI::Environment::set_floor_grid(bool enabled) const {
    // TODO: Implement floor grid rendering control
    if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
        logger->warning("[CoronaEngineAPI::Environment::set_floor_grid] Not implemented yet: " +
                        std::string(enabled ? "enabled" : "disabled"));
    }
}
