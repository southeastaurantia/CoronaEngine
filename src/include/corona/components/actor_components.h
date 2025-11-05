#pragma once

namespace Corona::Components {

struct ModelResource {
    std::uintptr_t model_handle;
    ResourceId model_id;
    std::uintptr_t device_handle;
};

}  // namespace Corona::Components
