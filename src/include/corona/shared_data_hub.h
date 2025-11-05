#pragma once
#include <corona/kernel/utils/storage.h>
#include <memory>
#include <vector>
#include <CabbageHardware.h>

// Forward declarations
#include "Mesh.h"

namespace Corona {

class Model;

struct ModelDevice {
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

    using ModelDeviceStorage = Kernel::Utils::Storage<std::vector<ModelDevice>>;
    ModelDeviceStorage& model_device_storage();
    const ModelDeviceStorage& model_device_storage() const;


   private:
    ModelDeviceStorage model_device_storage_;
    ModelStorage model_storage_;
};

}  // namespace Corona