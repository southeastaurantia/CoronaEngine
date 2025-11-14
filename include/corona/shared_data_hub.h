#pragma once
#include <corona/kernel/utils/storage.h>

#include <memory>
#include <vector>

#include "CabbageHardware.h"
#include "Mesh.h"

// Forward declarations

namespace Corona {

class Model;

struct MeshDevice {
    HardwareBuffer pointsBuffer;
    HardwareBuffer normalsBuffer;
    HardwareBuffer texCoordsBuffer;
    HardwareBuffer indexBuffer;
    HardwareBuffer boneIndexesBuffer;
    HardwareBuffer boneWeightsBuffer;

    uint32_t materialIndex;
    uint32_t textureIndex;

    Mesh meshData;
};

struct AnimationState {
    std::uint32_t animation_index = 0;
    float current_time = 0.0f;
    float playback_speed = 1.0f;
    bool active = true;
};

struct ModelTransform {
    ktm::fvec3 position;
    ktm::fvec3 euler_rotation;
    ktm::fvec3 scale;

    ModelTransform() {
        position.x = 0.0f;
        position.y = 0.0f;
        position.z = 0.0f;

        euler_rotation.x = 0.0f;
        euler_rotation.y = 0.0f;
        euler_rotation.z = 0.0f;

        scale.x = 1.0f;
        scale.y = 1.0f;
        scale.z = 1.0f;
    }

    [[nodiscard]] ktm::fmat4x4 compute_matrix() const {
        ktm::fquat qx = ktm::fquat::from_angle_x(euler_rotation.x);
        ktm::fquat qy = ktm::fquat::from_angle_y(euler_rotation.y);
        ktm::fquat qz = ktm::fquat::from_angle_z(euler_rotation.z);
        ktm::fquat rot_quat = qz * qy * qx;

        ktm::faffine3d affine;
        affine.translate(position).rotate(rot_quat).scale(scale);

        ktm::fmat4x4 result;
        affine >> result;
        return result;
    }
};

struct ModelResource {
    std::shared_ptr<Model> model_ptr;
};

struct SkinningDevice {
    HardwareBuffer bone_matrix_buffer;
};

struct GeometryDevice {
    std::uintptr_t transform_handle{};
    std::uintptr_t model_resource_handle{};
    std::vector<MeshDevice> mesh_handles;
};

struct KinematicsDevice {
    std::uintptr_t geometry_handle{};
    std::uintptr_t skinning_handle{};
    std::uintptr_t animation_controller_handle{};
};

struct MechanicsDevice {
    std::uintptr_t geometry_handle{};
    ktm::fvec3 max_xyz;
    ktm::fvec3 min_xyz;
};

struct AcousticsDevice {
    std::uintptr_t geometry_handle{};
    float volume{1.0f};
};

struct OpticsDevice {
    std::uintptr_t geometry_handle{};
    std::uintptr_t skinning_handle{};
};

struct ProfileDevice {
    std::uintptr_t optics_handle{};
    std::uintptr_t acoustics_handle{};
    std::uintptr_t mechanics_handle{};
    std::uintptr_t kinematics_handle{};
    std::uintptr_t geometry_handle{};
};

struct ActorDevice {
    std::vector<std::uintptr_t> profile_handles;
};

struct CameraDevice {
    void* surface{};

    ktm::fvec3 position;
    ktm::fvec3 forward;
    ktm::fvec3 world_up;
    float fov{60.0f};
    float aspect{16.0f/9.0f};
    float near_plane{0.1f};
    float far_plane{100.0f};

    CameraDevice() {
        position.x = 0.0f;
        position.y = 0.0f;
        position.z = 5.0f;

        forward.x = 0.0f;
        forward.y = 0.0f;
        forward.z = -1.0f;

        world_up.x = 0.0f;
        world_up.y = 1.0f;
        world_up.z = 0.0f;
    }

    [[nodiscard]] ktm::fmat4x4 compute_view_matrix() const {
        ktm::fvec3 normalized_forward = ktm::normalize(forward);
        return ktm::look_at_lh(position, position + normalized_forward, world_up);
    }

    [[nodiscard]] ktm::fmat4x4 compute_projection_matrix() const {
        return ktm::perspective_lh(ktm::radians(fov), aspect, near_plane, far_plane);
    }

    [[nodiscard]] ktm::fmat4x4 compute_view_proj_matrix() const {
        return compute_projection_matrix() * compute_view_matrix();
    }
};

struct ViewportDevice {
    std::uintptr_t camera{};
};

struct EnvironmentDevice {
    ktm::fvec3 sun_position;
};

struct SceneDevice {
    std::uintptr_t environment{};
    std::vector<std::uintptr_t> actor_handles;
    std::vector<std::uintptr_t> viewport_handles;
};

class SharedDataHub {
   public:
    static SharedDataHub& instance();

    ~SharedDataHub() = default;

    SharedDataHub(const SharedDataHub&) = delete;
    SharedDataHub& operator=(const SharedDataHub&) = delete;
    SharedDataHub(SharedDataHub&&) = delete;
    SharedDataHub& operator=(SharedDataHub&&) = delete;

   private:
    SharedDataHub() = default;

   public:
    using ModelResourceStorage = Kernel::Utils::Storage<ModelResource>;
    using ModelTransformStorage = Kernel::Utils::Storage<ModelTransform>;
    using AnimationControllerStorage = Kernel::Utils::Storage<AnimationState>;
    using SkinningStorage = Kernel::Utils::Storage<SkinningDevice>;
    using GeometryStorage = Kernel::Utils::Storage<GeometryDevice>;
    using KinematicsStorage = Kernel::Utils::Storage<KinematicsDevice>;
    using MechanicsStorage = Kernel::Utils::Storage<MechanicsDevice>;
    using AcousticsStorage = Kernel::Utils::Storage<AcousticsDevice>;
    using OpticsStorage = Kernel::Utils::Storage<OpticsDevice>;
    using ProfileStorage = Kernel::Utils::Storage<ProfileDevice>;
    using ActorStorage = Kernel::Utils::Storage<ActorDevice>;
    using CameraStorage = Kernel::Utils::Storage<CameraDevice>;
    using ViewportStorage = Kernel::Utils::Storage<ViewportDevice>;
    using EnvironmentStorage = Kernel::Utils::Storage<EnvironmentDevice>;
    using SceneStorage = Kernel::Utils::Storage<SceneDevice>;

    ModelResourceStorage& model_resource_storage();
    const ModelResourceStorage& model_resource_storage() const;

    ModelTransformStorage& model_transform_storage();
    const ModelTransformStorage& model_transform_storage() const;

    AnimationControllerStorage& animation_controller_storage();
    const AnimationControllerStorage& animation_controller_storage() const;

    SkinningStorage& skinning_storage();
    const SkinningStorage& skinning_storage() const;

    GeometryStorage& geometry_storage();
    const GeometryStorage& geometry_storage() const;

    KinematicsStorage& kinematics_storage();
    const KinematicsStorage& kinematics_storage() const;

    MechanicsStorage& mechanics_storage();
    const MechanicsStorage& mechanics_storage() const;

    AcousticsStorage& acoustics_storage();
    const AcousticsStorage& acoustics_storage() const;

    OpticsStorage& optics_storage();
    const OpticsStorage& optics_storage() const;

    ProfileStorage& profile_storage();
    const ProfileStorage& profile_storage() const;

    ActorStorage& actor_storage();
    const ActorStorage& actor_storage() const;

    CameraStorage& camera_storage();
    const CameraStorage& camera_storage() const;

    ViewportStorage& viewport_storage();
    const ViewportStorage& viewport_storage() const;

    EnvironmentStorage& environment_storage();
    const EnvironmentStorage& environment_storage() const;

    SceneStorage& scene_storage();
    const SceneStorage& scene_storage() const;

   private:
    ModelResourceStorage model_resource_storage_;

    GeometryStorage geometry_storage_;
    ModelTransformStorage model_transform_storage_;

    SkinningStorage skinning_storage_;
    AnimationControllerStorage animation_controller_storage_;

    KinematicsStorage kinematics_storage_;
    MechanicsStorage mechanics_storage_;

    OpticsStorage optics_storage_;
    AcousticsStorage acoustics_storage_;

    ProfileStorage profile_storage_;
    ActorStorage actor_storage_;

    EnvironmentStorage environment_storage_;
    CameraStorage camera_storage_;
    ViewportStorage viewport_storage_;
    SceneStorage scene_storage_;
};

}  // namespace Corona
