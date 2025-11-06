//
// Created by 25473 on 25-9-19.
//

#include <ResourceManager.h>
#include <corona/api/corona_engine_api.h>
#include <corona/components/actor_components.h>
#include <corona/components/scene_components.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/kernel_context.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/shared_data_hub.h>

#include "Model.h"
#include <CabbageHardware.h>

// 定义静态 ECS 注册表
entt::registry CoronaEngineAPI::registry_;

CoronaEngineAPI::Scene::Scene(void* surface, bool /*lightField*/)
    : scene_id_(registry_.create()) {
    registry_.emplace<RenderTag>(scene_id_);
    if (surface) {
        registry_.emplace_or_replace<Corona::Components::DisplaySurface>(scene_id_, Corona::Components::DisplaySurface{surface});
        if (auto* event_bus = Corona::Kernel::KernelContext::instance().event_bus()) {
            event_bus->publish<Corona::Events::DisplaySurfaceChangedEvent>({surface});
        }
    }
}

CoronaEngineAPI::Scene::~Scene() {
    registry_.destroy(scene_id_);
}

void CoronaEngineAPI::Scene::set_camera(const ktm::fvec3& position, const ktm::fvec3& forward, const ktm::fvec3& world_up, float fov) const {
    registry_.emplace_or_replace<Corona::Components::Camera>(scene_id_, Corona::Components::Camera{fov, position, forward,  world_up});
}

void CoronaEngineAPI::Scene::set_sun_direction(ktm::fvec3 direction) const {
    registry_.emplace_or_replace<Corona::Components::SunDirection>(scene_id_, Corona::Components::SunDirection{direction});
}

void CoronaEngineAPI::Scene::set_display_surface(void* surface) {
    registry_.emplace_or_replace<Corona::Components::DisplaySurface>(scene_id_, Corona::Components::DisplaySurface{surface});

    // 获取 EventBus 并发布事件
    if (auto* event_bus = Corona::Kernel::KernelContext::instance().event_bus()) {
        event_bus->publish<Corona::Events::DisplaySurfaceChangedEvent>({surface});
    }
}

CoronaEngineAPI::Actor::Actor(const std::string& path)
    : actor_id_(registry_.create()), model_handle_(0), device_handle_(0) {
    registry_.emplace<RenderTag>(actor_id_);
    auto model_id = Corona::ResourceId::from("model", path);
    auto model_ptr = std::static_pointer_cast<Corona::Model>(Corona::ResourceManager::instance().load_once(model_id));
    model_handle_ = Corona::SharedDataHub::instance().model_storage().allocate([&](std::shared_ptr<Corona::Model>& slot) {
       slot = model_ptr;
    });

    std::vector<Corona::ModelDevice> devices;
    devices.reserve(model_ptr->meshes.size());
    for (const auto & mesh : model_ptr->meshes) {
        devices.emplace_back(Corona::ModelDevice{
            .pointsBuffer = HardwareBuffer(mesh.points, BufferUsage::VertexBuffer),
            .normalsBuffer = HardwareBuffer(mesh.normals, BufferUsage::VertexBuffer),
            .texCoordsBuffer = HardwareBuffer(mesh.texCoords, BufferUsage::VertexBuffer),
            .indexBuffer = HardwareBuffer(mesh.Indices, BufferUsage::IndexBuffer),
            .boneIndexesBuffer = HardwareBuffer(mesh.boneIndices, BufferUsage::VertexBuffer),
            .boneWeightsBuffer = HardwareBuffer(mesh.boneWeights, BufferUsage::VertexBuffer),
            .materialIndex = 0,
            .textureIndex = HardwareImage(mesh.textures[0]->width, mesh.textures[0]->height, ImageFormat::RGBA8_SRGB, ImageUsage::SampledImage, 1, mesh.textures[0]->data),
            .meshData = mesh
        });
    }

    device_handle_ = Corona::SharedDataHub::instance().model_device_storage().allocate([&](std::vector<Corona::ModelDevice>& slot) {
        slot = std::move(devices);
    });

    registry_.emplace_or_replace<Corona::Components::ModelResource>(actor_id_, Corona::Components::ModelResource{model_handle_, model_id, device_handle_});
}

CoronaEngineAPI::Actor::~Actor() {
    if (model_handle_) {
        Corona::SharedDataHub::instance().model_storage().deallocate(model_handle_);
    }
    if (device_handle_) {
        Corona::SharedDataHub::instance().model_device_storage().deallocate(device_handle_);
    }
    registry_.destroy(actor_id_);
}

void CoronaEngineAPI::Actor::move(ktm::fvec3 /*pos*/) const {
}

void CoronaEngineAPI::Actor::rotate(ktm::fvec3 /*euler*/) const {
}

void CoronaEngineAPI::Actor::scale(ktm::fvec3 /*size*/) const {
}
