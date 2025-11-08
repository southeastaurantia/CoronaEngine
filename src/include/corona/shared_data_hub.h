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
    std::uintptr_t model_handle{};
    std::uintptr_t transform_handle{};
    std::uint32_t animation_index = 0;
    float current_time = 0.0f;
    bool active = true;
};

struct ModelTransform {
    ktm::fmat4x4 model_matrix;
};

struct ModelBounding {
    std::uintptr_t transform_handle{};
    ktm::fvec3 max_xyz;
    ktm::fvec3 min_xyz;
};

struct ModelDevice {
    std::uintptr_t transform_handle{};
    std::uintptr_t animation_handle{};
    HardwareBuffer bone_matrix_buffer;
    std::vector<MeshDevice> devices;
};

struct CameraDevice {
    std::uint64_t surface{};
    ktm::fvec3 eye_position;
    ktm::fvec3 eye_dir;
    ktm::fmat4x4 eye_view_matrix;
    ktm::fmat4x4 eye_proj_matrix;
    ktm::fmat4x4 view_proj_matrix;
};

struct LightDevice {
};

struct SceneDevice {
    ktm::fvec3 sun_direction;
    std::vector<std::uintptr_t> actors;
    std::vector<std::uintptr_t> cameras;
    std::vector<std::uintptr_t> lights;
};

class SharedDataHub {
   public:
    static SharedDataHub& instance();

    ~SharedDataHub() = default;

   private:
    SharedDataHub() = default;
    SharedDataHub(const SharedDataHub&) = delete;
    SharedDataHub& operator=(const SharedDataHub&) = delete;
    SharedDataHub(SharedDataHub&&) = delete;
    SharedDataHub& operator=(SharedDataHub&&) = delete;

   public:
    using ModelStorage = Kernel::Utils::Storage<std::shared_ptr<Model>>;
    ModelStorage& model_storage();
    const ModelStorage& model_storage() const;

    using SceneStorage = Kernel::Utils::Storage<SceneDevice>;
    SceneStorage& scene_storage();
    const SceneStorage& scene_storage() const;

    using CameraStorage = Kernel::Utils::Storage<CameraDevice>;
    CameraStorage& camera_storage();
    const CameraStorage& camera_storage() const;

    using LightStorage = Kernel::Utils::Storage<LightDevice>;
    LightStorage& light_storage();
    const LightStorage& light_storage() const;

    using ModelTransformStorage = Kernel::Utils::Storage<ModelTransform>;
    ModelTransformStorage& model_transform_storage();
    const ModelTransformStorage& model_transform_storage() const;

    using ModelDeviceStorage = Kernel::Utils::Storage<ModelDevice>;
    ModelDeviceStorage& model_device_storage();
    const ModelDeviceStorage& model_device_storage() const;

    using ModelBoundingStorage = Kernel::Utils::Storage<ModelBounding>;
    ModelBoundingStorage& model_bounding_storage();
    const ModelBoundingStorage& model_bounding_storage() const;

    using AnimationStateStorage = Kernel::Utils::Storage<AnimationState>;
    AnimationStateStorage& animation_state_storage();
    const AnimationStateStorage& animation_state_storage() const;

   private:
    ModelDeviceStorage model_device_storage_;
    ModelBoundingStorage model_bounding_storage_;
    ModelTransformStorage model_transform_storage_;
    AnimationStateStorage animation_state_storage_;
    ModelStorage model_storage_;
    SceneStorage scene_storage_;
    CameraStorage camera_storage_;
    LightStorage light_storage_;
};

}  // namespace Corona