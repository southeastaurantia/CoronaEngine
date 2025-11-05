#include <corona/shared_data_hub.h>

namespace Corona {

SharedDataHub& SharedDataHub::instance() {
    static SharedDataHub instance;
    return instance;
}


SharedDataHub::ModelStorage& SharedDataHub::model_storage() {
    return model_storage_;
}

const SharedDataHub::ModelStorage& SharedDataHub::model_storage() const {
    return this->model_storage_;
}

SharedDataHub::ModelDeviceStorage& SharedDataHub::model_device_storage() {
    return this->model_device_storage_;
}

const SharedDataHub::ModelDeviceStorage& SharedDataHub::model_device_storage() const {
    return this->model_device_storage_;
}

}  // namespace Corona