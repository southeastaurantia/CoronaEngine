#include <corona/events/engine_events.h>
#include <corona/events/mechanics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/mechanics_system.h>

#include "corona/shared_data_hub.h"
#include "ktm/ktm.h"

namespace {
std::vector<ktm::fvec3> calculateVertices(const ktm::fvec3& startMin, const ktm::fvec3& startMax) {
    std::vector<ktm::fvec3> vertices;
    vertices.reserve(8);
    vertices.push_back(startMin);
    vertices.emplace_back(startMax.x, startMin.y, startMin.z);
    vertices.emplace_back(startMin.x, startMax.y, startMin.z);
    vertices.emplace_back(startMax.x, startMax.y, startMin.z);
    vertices.emplace_back(startMin.x, startMin.y, startMax.z);
    vertices.emplace_back(startMax.x, startMin.y, startMax.z);
    vertices.emplace_back(startMin.x, startMax.y, startMax.z);
    vertices.push_back(startMax);
    return vertices;
}

bool checkCollision(const std::vector<ktm::fvec3>& vertices1, const std::vector<ktm::fvec3>& vertices2) {
    // 计算vertices2的AABB
    ktm::fvec3 min2 = vertices2[0], max2 = vertices2[0];
    for (const auto& v : vertices2) {
        min2 = ktm::min(min2, v);
        max2 = ktm::max(max2, v);
    }

    // 检查vertices1的顶点是否在vertices2的AABB内
    for (const auto& point : vertices1) {
        if (point.x >= min2.x && point.x <= max2.x &&
            point.y >= min2.y && point.y <= max2.y &&
            point.z >= min2.z && point.z <= max2.z) {
            return true;
            }
    }

    // 计算vertices1的AABB
    ktm::fvec3 min1 = vertices1[0], max1 = vertices1[0];
    for (const auto& v : vertices1) {
        min1 = ktm::min(min1, v);
        max1 = ktm::max(max1, v);
    }

    // 检查vertices2的顶点是否在vertices1的AABB内
    for (const auto& point : vertices2) {
        if (point.x >= min1.x && point.x <= max1.x &&
            point.y >= min1.y && point.y <= max1.y &&
            point.z >= min1.z && point.z <= max1.z) {
            return true;
            }
    }

    return false;
}
}  // namespace


namespace Corona::Systems {

bool MechanicsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("MechanicsSystem: Initializing...");
    return true;
}

void MechanicsSystem::update() {

    SharedDataHub::instance().model_bounding_storage().for_each_read([&](const ModelBounding& model) {
       // SharedDataHub::instance().model_bounding_storage().for_each_read([&](const ModelBounding& other_model) {
       //     if (&model == &other_model) {
       //         return;
       //     }
       //
       //     auto StartMin = model.min_xyz;
       //     auto StartMax = model.max_xyz;
       //
       //     // 计算世界坐标下的包围盒顶点
       //     std::vector<ktm::fvec3> Vertices1 = calculateVertices(StartMin, StartMax);
       //     ktm::fmat4x4 actorMatrix = model.modelMatrix;
       //     for (auto& v : Vertices1) {
       //         v = ktm::fvec4(actorMatrix * ktm::fvec4(v, 1.0f)).xyz();
       //     }
       //
       //     auto otherStartMin = otherM.minXYZ;
       //     auto otherStartMax = otherM.maxXYZ;
       //
       //     // 计算世界坐标下的包围盒顶点
       //     std::vector<ktm::fvec3> Vertices2 = calculateVertices(otherStartMin, otherStartMax);
       //     ktm::fmat4x4 otherModelMatrix = otherM.modelMatrix;
       //     for (auto& v : Vertices2) {
       //         v = ktm::fvec4(otherModelMatrix * ktm::fvec4(v, 1.0f)).xyz();
       //     }
       //
       //     // 碰撞检测
       //     if (checkCollision(Vertices1, Vertices2)) {
       //         CE_LOG_INFO("Collision detected!");
       //         collisionActors_.insert(&otherM);
       //         collisionActors_.insert(&m);
       //     }
       // });
    });

}

void MechanicsSystem::update_physics() {
    // collisionActors_.clear();
    //
    // auto StartMin = m.minXYZ;
    // auto StartMax = m.maxXYZ;
    //
    // // 计算世界坐标下的包围盒顶点
    // std::vector<ktm::fvec3> Vertices1 = calculateVertices(StartMin, StartMax);
    // ktm::fmat4x4 actorMatrix = m.modelMatrix;
    // for (auto& v : Vertices1) {
    //     v = ktm::fvec4(actorMatrix * ktm::fvec4(v, 1.0f)).xyz();
    // }
    //
    // if (auto* caches = data_caches()) {
    //     auto& model_cache = caches->get<Model>();
    //     model_cache.safe_loop_foreach(other_model_cache_keys_, [&](std::shared_ptr<Model> otherModel) {
    //         if (!otherModel) {
    //             return;
    //         }
    //
    //         auto otherStartMin = otherModel->minXYZ;
    //         auto otherStartMax = otherModel->maxXYZ;
    //
    //         // 计算世界坐标下的包围盒顶点
    //         std::vector<ktm::fvec3> Vertices2 = calculateVertices(otherStartMin, otherStartMax);
    //         ktm::fmat4x4 otherModelMatrix = otherModel->modelMatrix;
    //         for (auto& v : Vertices2) {
    //             v = ktm::fvec4(otherModelMatrix * ktm::fvec4(v, 1.0f)).xyz();
    //         }
    //
    //         // 碰撞检测
    //         if (checkCollision(Vertices1, Vertices2)) {
    //             CE_LOG_INFO("Collision detected!");
    //             collisionActors_.insert(otherModel.get());
    //             collisionActors_.insert(std::addressof(m));
    //
    //             // 碰撞响应
    //             // 计算碰撞法线（从actor指向otherActor）
    //             ktm::fvec3 center1 = (StartMin + StartMax) * 0.5f;
    //             ktm::fvec3 center2 = (otherStartMin + otherStartMax) * 0.5f;
    //             ktm::fvec3 normal = ktm::normalize(center2 - center1);
    //
    //             // 物体分离（防止穿透）
    //             const float separation = 0.02f;
    //             m.positon += ktm::fvec3(-normal * separation);
    //             otherModel->positon += (normal * separation);
    //
    //             // 直接施加反弹位移
    //             const float bounceStrength = 0.1f;  // 反弹强度
    //             m.positon += ktm::fvec3(-normal * bounceStrength);
    //             otherModel->positon += (normal * bounceStrength);
    //
    //             // 更新模型矩阵           /*变换矩阵 = 平移 * 旋转 * 缩放/     //先缩放在旋转后平移//
    //             m.modelMatrix = ktm::fmat4x4(ktm::translate3d(m.positon) * ktm::translate3d(m.rotation) * ktm::translate3d(m.scale));
    //             otherModel->modelMatrix = ktm::fmat4x4(ktm::translate3d(otherModel->positon) * ktm::translate3d(otherModel->rotation) * ktm::translate3d(otherModel->scale));
    //         }
    //     });
    // }
}

void MechanicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("MechanicsSystem: Shutting down...");
}

}  // namespace Corona::Systems
