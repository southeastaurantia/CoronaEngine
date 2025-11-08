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

SharedDataHub::SceneStorage& SharedDataHub::scene_storage() {
    return this->scene_storage_;
}

const SharedDataHub::SceneStorage& SharedDataHub::scene_storage() const {
    return this->scene_storage_;
}

SharedDataHub::CameraStorage& SharedDataHub::camera_storage() {
    return this->camera_storage_;
}

const SharedDataHub::CameraStorage& SharedDataHub::camera_storage() const {
    return this->camera_storage_;
}

SharedDataHub::LightStorage& SharedDataHub::light_storage() {
    return this->light_storage_;
}

const SharedDataHub::LightStorage& SharedDataHub::light_storage() const {
    return this->light_storage_;
}

SharedDataHub::ModelBoundingStorage& SharedDataHub::model_bounding_storage() {
    return this->model_bounding_storage_;
}

const SharedDataHub::ModelBoundingStorage& SharedDataHub::model_bounding_storage() const {
    return this->model_bounding_storage_;
}

SharedDataHub::ModelTransformStorage& SharedDataHub::model_transform_storage() {
    return this->model_transform_storage_;
}

const SharedDataHub::ModelTransformStorage& SharedDataHub::model_transform_storage() const {
    return this->model_transform_storage_;
}

SharedDataHub::AnimationStateStorage& SharedDataHub::animation_state_storage() {
    return this->animation_state_storage_;
}

const SharedDataHub::AnimationStateStorage& SharedDataHub::animation_state_storage() const {
    return this->animation_state_storage_;
}

SharedDataHub::BoneMatrixStorage& SharedDataHub::bone_matrix_storage() {
    return this->bone_matrix_storage_;
}

const SharedDataHub::BoneMatrixStorage& SharedDataHub::bone_matrix_storage() const {
    return this->bone_matrix_storage_;
}

}  // namespace Corona