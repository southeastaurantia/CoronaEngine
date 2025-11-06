//
// Created by 25473 on 25-9-19.
//

#include <CabbageHardware.h>
#include <ResourceManager.h>
#include <corona/api/corona_engine_api.h>
#include <corona/components/actor_components.h>
#include <corona/components/scene_components.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/kernel_context.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/shared_data_hub.h>

#include "Model.h"

// 定义静态 ECS 注册表
entt::registry CoronaEngineAPI::registry_;

// ########################
//          Scene
// ########################
CoronaEngineAPI::Scene::Scene(bool lightField)
    : scene_id_(registry_.create()), scene_handle_(0) {
    registry_.emplace<RenderTag>(scene_id_);

    scene_handle_ = Corona::SharedDataHub::instance().scene_storage().allocate([&](Corona::SceneDevice& slot) {
        slot.sun_direction = ktm::fvec3{0.0f, -1.0f, 0.0f};
    });
}

CoronaEngineAPI::Scene::~Scene() {
    if (scene_handle_ != 0) {
        Corona::SharedDataHub::instance().scene_storage().deallocate(scene_handle_);
    }
    registry_.destroy(scene_id_);
}

// void CoronaEngineAPI::Scene::set_camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov) const {
//     registry_.emplace_or_replace<Corona::Components::Camera>(scene_id_, Corona::Components::Camera{fov, position, forward,  world_up});
// }

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
    : Base(), model_handle_(0), device_handle_(0) {
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
    actor.position = ktm::fvec3{0.0f, 0.0f, 0.0f};
    actor.rotation = ktm::fvec3{0.0f, 0.0f, 0.0f};
    actor.scale = ktm::fvec3{1.0f, 1.0f, 1.0f};

    model_handle_ = Corona::SharedDataHub::instance().model_storage().allocate([&](std::shared_ptr<Corona::Model>& slot) {
        slot = model_ptr;
    });

    if (model_handle_ == 0) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::Actor] model_storage allocation failed for: " + path);
        }
        return;
    }

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

    device_handle_ = Corona::SharedDataHub::instance().model_device_storage().allocate([&](Corona::ModelDevice& slot) {
        slot.modelMatrix = ktm::fmat4x4(ktm::translate3d(actor.position) * ktm::translate3d(actor.rotation) * ktm::translate3d(actor.scale));
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
    if (model_handle_) {
        Corona::SharedDataHub::instance().model_storage().deallocate(model_handle_);
    }
    if (device_handle_) {
        Corona::SharedDataHub::instance().model_device_storage().deallocate(device_handle_);
    }
    registry_.destroy(id_);
}

std::uintptr_t CoronaEngineAPI::Actor::get_handle_id() const {
    return model_handle_;
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 pos) const {
    auto& actor = registry_.get<Corona::Components::Actor>(id_);
    actor.position = pos;
    bool info = Corona::SharedDataHub::instance().model_device_storage().write(device_handle_, [&](Corona::ModelDevice& slot) {
        slot.modelMatrix = ktm::fmat4x4(ktm::translate3d(actor.position) * ktm::translate3d(actor.rotation) * ktm::translate3d(actor.scale));
    });

    if (!info) {
        if (auto* logger = Corona::Kernel::KernelContext::instance().logger()) {
            logger->error("[CoronaEngineAPI::Actor::move] model_device_storage error");
        }
    }
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 euler) const {
    auto& actor = registry_.get<Corona::Components::Actor>(id_);
    actor.rotation = euler;
    bool info = Corona::SharedDataHub::instance().model_device_storage().write(device_handle_, [&](Corona::ModelDevice& slot) {
        slot.modelMatrix = ktm::fmat4x4(ktm::translate3d(actor.position) * ktm::translate3d(actor.rotation) * ktm::translate3d(actor.scale));
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
    bool info = Corona::SharedDataHub::instance().model_device_storage().write(device_handle_, [&](Corona::ModelDevice& slot) {
        slot.modelMatrix = ktm::fmat4x4(ktm::translate3d(actor.position) * ktm::translate3d(actor.rotation) * ktm::translate3d(actor.scale));
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
    : Base(), handle_(0) {
    ktm::fvec3 position{0.0f, 0.0f, 5.0f};
    ktm::fvec3 forward{0.0f, 0.0f, -1.0f};
    ktm::fvec3 world_up{0.0f, 1.0f, 0.0f};
    float fov = 45.0f;

    registry_.emplace<Corona::Components::Camera>(id_, Corona::Components::Camera{position, forward, world_up, fov});

    handle_ = Corona::SharedDataHub::instance().camera_storage().allocate([&](Corona::CameraDevice& slot) {
        slot.eyePosition = position;
        slot.eyeDir = ktm::normalize(forward);
        slot.eyeViewMatrix = ktm::look_at_lh(position, ktm::normalize(forward), world_up);
        slot.eyeProjMatrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
    });
}

CoronaEngineAPI::Camera::Camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov)
    : Base(), handle_(0) {
    registry_.emplace<Corona::Components::Camera>(id_, Corona::Components::Camera{position, forward, world_up, fov});

    handle_ = Corona::SharedDataHub::instance().camera_storage().allocate([&](Corona::CameraDevice& slot) {
        slot.eyePosition = position;
        slot.eyeDir = ktm::normalize(forward);
        slot.eyeViewMatrix = ktm::look_at_lh(position, ktm::normalize(forward), world_up);
        slot.eyeProjMatrix = ktm::perspective_lh(ktm::radians(fov), 16.0f / 9.0f, 0.1f, 100.0f);
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
}

// ########################
//          Light
// ########################
CoronaEngineAPI::Light::Light()
    : Base(), handle_(0) {
    registry_.emplace<Corona::Components::Light>(id_, Corona::Components::Light{ktm::fvec3{0.0f, 5.0f, 0.0f}, ktm::fvec3{1.0f, 1.0f, 1.0f}, ktm::fvec3{0.0f, 0.0f, 0.0f}});
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
