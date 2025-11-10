#include <corona/events/engine_events.h>
#include <corona/events/mechanics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/mechanics/mechanics_system.h>

#include "corona/shared_data_hub.h"
#include "ktm/ktm.h"

namespace {
std::vector<ktm::fvec3> calculateVertices(const ktm::fvec3& startMin, const ktm::fvec3& startMax) {
    std::vector<ktm::fvec3> vertices;
    vertices.reserve(8);

    vertices.push_back(startMin);

    ktm::fvec3 v1;
    v1.x = startMax.x;
    v1.y = startMin.y;
    v1.z = startMin.z;
    vertices.push_back(v1);

    ktm::fvec3 v2;
    v2.x = startMin.x;
    v2.y = startMax.y;
    v2.z = startMin.z;
    vertices.push_back(v2);

    ktm::fvec3 v3;
    v3.x = startMax.x;
    v3.y = startMax.y;
    v3.z = startMin.z;
    vertices.push_back(v3);

    ktm::fvec3 v4;
    v4.x = startMin.x;
    v4.y = startMin.y;
    v4.z = startMax.z;
    vertices.push_back(v4);

    ktm::fvec3 v5;
    v5.x = startMax.x;
    v5.y = startMin.y;
    v5.z = startMax.z;
    vertices.push_back(v5);

    ktm::fvec3 v6;
    v6.x = startMin.x;
    v6.y = startMax.y;
    v6.z = startMax.z;
    vertices.push_back(v6);

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
    update_physics();
}

void MechanicsSystem::update_physics() {
    auto* logger = context()->logger();

    // 直接使用 SharedDataHub 进行碰撞检测，不需要额外的数据结构
    SharedDataHub::instance().model_bounding_storage().for_each_read([&](const ModelBounding& bounding1) {
        SharedDataHub::instance().model_bounding_storage().for_each_read([&](const ModelBounding& bounding2) {
            // 跳过自己与自己的比较
            if (&bounding1 == &bounding2) {
                return;
            }

            // 获取第一个模型的变换矩阵和世界坐标顶点
            std::vector<ktm::fvec3> vertices1;
            bool found1 = SharedDataHub::instance().model_transform_storage().read(
                bounding1.transform_handle,
                [&](const ModelTransform& transform) {
                    // 计算本地空间的包围盒顶点
                    vertices1 = calculateVertices(bounding1.min_xyz, bounding1.max_xyz);

                    // 变换到世界坐标
                    for (auto& v : vertices1) {
                        ktm::fvec4 v4;
                        v4.x = v.x;
                        v4.y = v.y;
                        v4.z = v.z;
                        v4.w = 1.0f;

                        ktm::fvec4 world_v = transform.model_matrix * v4;
                        v.x = world_v.x;
                        v.y = world_v.y;
                        v.z = world_v.z;
                    }
                });

            if (!found1 || vertices1.empty()) {
                return;
            }

            // 获取第二个模型的变换矩阵和世界坐标顶点
            std::vector<ktm::fvec3> vertices2;
            bool found2 = SharedDataHub::instance().model_transform_storage().read(
                bounding2.transform_handle,
                [&](const ModelTransform& transform) {
                    vertices2 = calculateVertices(bounding2.min_xyz, bounding2.max_xyz);

                    for (auto& v : vertices2) {
                        ktm::fvec4 v4;
                        v4.x = v.x;
                        v4.y = v.y;
                        v4.z = v.z;
                        v4.w = 1.0f;

                        ktm::fvec4 world_v = transform.model_matrix * v4;
                        v.x = world_v.x;
                        v.y = world_v.y;
                        v.z = world_v.z;
                    }
                });

            if (!found2 || vertices2.empty()) {
                return;
            }

            // 碰撞检测
            if (checkCollision(vertices1, vertices2)) {
                if (logger) {
                    logger->info("Collision detected!");
                }

                // 计算碰撞法线（从 bounding1 指向 bounding2）
                ktm::fvec3 center1;
                center1.x = (bounding1.min_xyz.x + bounding1.max_xyz.x) * 0.5f;
                center1.y = (bounding1.min_xyz.y + bounding1.max_xyz.y) * 0.5f;
                center1.z = (bounding1.min_xyz.z + bounding1.max_xyz.z) * 0.5f;

                ktm::fvec3 center2;
                center2.x = (bounding2.min_xyz.x + bounding2.max_xyz.x) * 0.5f;
                center2.y = (bounding2.min_xyz.y + bounding2.max_xyz.y) * 0.5f;
                center2.z = (bounding2.min_xyz.z + bounding2.max_xyz.z) * 0.5f;

                ktm::fvec3 diff;
                diff.x = center2.x - center1.x;
                diff.y = center2.y - center1.y;
                diff.z = center2.z - center1.z;

                ktm::fvec3 normal = ktm::normalize(diff);

                // 物体分离（防止穿透）
                const float separation = 0.02f;
                const float bounceStrength = 0.1f;

                ktm::fvec3 total_offset;
                total_offset.x = normal.x * (separation + bounceStrength);
                total_offset.y = normal.y * (separation + bounceStrength);
                total_offset.z = normal.z * (separation + bounceStrength);

                // 更新第一个模型的位置（反方向）
                bool updated1 = SharedDataHub::instance().model_transform_storage().write(
                    bounding1.transform_handle,
                    [&](ModelTransform& transform) {
                        ktm::faffine3d offset_transform;
                        ktm::fvec3 neg_offset;
                        neg_offset.x = -total_offset.x;
                        neg_offset.y = -total_offset.y;
                        neg_offset.z = -total_offset.z;
                        offset_transform.translate(neg_offset);

                        ktm::fmat4x4 offset_matrix;
                        offset_transform >> offset_matrix;

                        transform.model_matrix = transform.model_matrix * offset_matrix;
                    });

                // 更新第二个模型的位置（正方向）
                bool updated2 = SharedDataHub::instance().model_transform_storage().write(
                    bounding2.transform_handle,
                    [&](ModelTransform& transform) {
                        ktm::faffine3d offset_transform;
                        offset_transform.translate(total_offset);

                        ktm::fmat4x4 offset_matrix;
                        offset_transform >> offset_matrix;

                        transform.model_matrix = transform.model_matrix * offset_matrix;
                    });

                if (!updated1 || !updated2) {
                    if (logger) {
                        logger->warning("MechanicsSystem: Failed to update transform after collision");
                    }
                }
            }
        });
    });
}

void MechanicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("MechanicsSystem: Shutting down...");
}
}  // namespace Corona::Systems