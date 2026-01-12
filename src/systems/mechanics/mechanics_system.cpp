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
    CFW_LOG_NOTICE("MechanicsSystem: Initializing...");
    return true;
}

void MechanicsSystem::update() {
    update_physics();
}

void MechanicsSystem::update_physics() {
    auto& mechanics_storage = SharedDataHub::instance().mechanics_storage();
    auto& geometry_storage = SharedDataHub::instance().geometry_storage();
    auto& transform_storage = SharedDataHub::instance().model_transform_storage();
    
    // 获取所有的 MechanicsDevice 句柄
    std::vector<std::uintptr_t> mechanics_handles;
    
    // 遍历所有 MechanicsDevice 来收集句柄（假设有 for_each 或类似方法）
    // 如果没有遍历方法，我们需要维护一个句柄列表或使用其他方式获取所有活动句柄
    
    // 临时解决方案：假设我们有一种方式获取所有活动的句柄
    // 这里需要根据实际的 Storage API 进行调整
    
    // CFW_LOG_DEBUG("MechanicsSystem: Starting collision detection update");
    
    // 双重循环进行碰撞检测
    // 注意：这里需要根据实际的 Storage API 来获取所有活动句柄
    // 目前我们先实现基本框架，假设我们有方式获取句柄列表
    
    // 示例：如果我们有一些方式获取句柄列表
    for (std::uintptr_t handle1 : mechanics_handles) {
        // 获取第一个 MechanicsDevice 的读取访问权
        auto m1_accessor = mechanics_storage.acquire_read(handle1);
        if (!m1_accessor) {
            continue; // 无法获取访问权，跳过
        }
        
        const auto& m1 = *m1_accessor;
        
        for (std::uintptr_t handle2 : mechanics_handles) {
            if (handle1 == handle2) {
                continue; // 跳过自身
            }
            
            // 获取第二个 MechanicsDevice 的读取访问权
            auto m2_accessor = mechanics_storage.acquire_read(handle2);
            if (!m2_accessor) {
                continue;
            }
            
            const auto& m2 = *m2_accessor;
            
            // 获取第一个物体的世界坐标顶点
            std::vector<ktm::fvec3> vertices1;
            bool found1 = false;
            
            if (auto geom1_accessor = geometry_storage.acquire_read(m1.geometry_handle)) {
                const auto& geom1 = *geom1_accessor;
                
                if (auto transform1_accessor = transform_storage.acquire_read(geom1.transform_handle)) {
                    const auto& transform1 = *transform1_accessor;
                    
                    vertices1 = calculateVertices(m1.min_xyz, m1.max_xyz);
                    
                    // 从局部参数计算世界矩阵
                    ktm::fmat4x4 world_matrix = transform1.compute_matrix();
                    
                    for (auto& v : vertices1) {
                        ktm::fvec4 v4;
                        v4.x = v.x;
                        v4.y = v.y;
                        v4.z = v.z;
                        v4.w = 1.0f;
                        
                        ktm::fvec4 world_v = world_matrix * v4;
                        v.x = world_v.x;
                        v.y = world_v.y;
                        v.z = world_v.z;
                    }
                    found1 = true;
                }
            }
            
            if (!found1 || vertices1.empty()) {
                continue;
            }
            
            // 获取第二个物体的世界坐标顶点
            std::vector<ktm::fvec3> vertices2;
            bool found2 = false;
            
            if (auto geom2_accessor = geometry_storage.acquire_read(m2.geometry_handle)) {
                const auto& geom2 = *geom2_accessor;
                
                if (auto transform2_accessor = transform_storage.acquire_read(geom2.transform_handle)) {
                    const auto& transform2 = *transform2_accessor;
                    
                    vertices2 = calculateVertices(m2.min_xyz, m2.max_xyz);
                    
                    // 从局部参数计算世界矩阵
                    ktm::fmat4x4 world_matrix = transform2.compute_matrix();
                    
                    for (auto& v : vertices2) {
                        ktm::fvec4 v4;
                        v4.x = v.x;
                        v4.y = v.y;
                        v4.z = v.z;
                        v4.w = 1.0f;
                        
                        ktm::fvec4 world_v = world_matrix * v4;
                        v.x = world_v.x;
                        v.y = world_v.y;
                        v.z = world_v.z;
                    }
                    found2 = true;
                }
            }
            
            if (!found2 || vertices2.empty()) {
                continue;
            }
            
            // 碰撞检测
            if (checkCollision(vertices1, vertices2)) {
                // CFW_LOG_DEBUG("Collision detected between objects with handles {} and {}", handle1, handle2);
                
                // 计算碰撞法线（从 m1 中心指向 m2 中心）
                ktm::fvec3 center1;
                center1.x = (m1.min_xyz.x + m1.max_xyz.x) * 0.5f;
                center1.y = (m1.min_xyz.y + m1.max_xyz.y) * 0.5f;
                center1.z = (m1.min_xyz.z + m1.max_xyz.z) * 0.5f;
                
                ktm::fvec3 center2;
                center2.x = (m2.min_xyz.x + m2.max_xyz.x) * 0.5f;
                center2.y = (m2.min_xyz.y + m2.max_xyz.y) * 0.5f;
                center2.z = (m2.min_xyz.z + m2.max_xyz.z) * 0.5f;
                
                ktm::fvec3 diff;
                diff.x = center2.x - center1.x;
                diff.y = center2.y - center1.y;
                diff.z = center2.z - center1.z;
                
                ktm::fvec3 normal = ktm::normalize(diff);
                
                // 物体分离（防止穿透）
                constexpr float separation = 0.02f;
                constexpr float bounceStrength = 0.1f;
                
                ktm::fvec3 total_offset;
                total_offset.x = normal.x * (separation + bounceStrength);
                total_offset.y = normal.y * (separation + bounceStrength);
                total_offset.z = normal.z * (separation + bounceStrength);
                
                // 更新 m1 的 transform（反方向偏移）- 直接修改局部位置参数
                if (auto geom1_accessor = geometry_storage.acquire_read(m1.geometry_handle)) {
                    const auto& geom1 = *geom1_accessor;
                    
                    if (auto transform1_accessor = transform_storage.acquire_write(geom1.transform_handle)) {
                        auto& transform1 = *transform1_accessor;
                        
                        // 直接修改局部位置参数
                        transform1.position.x -= total_offset.x;
                        transform1.position.y -= total_offset.y;
                        transform1.position.z -= total_offset.z;
                    }
                }
                
                // 更新 m2 的 transform（正方向偏移）- 直接修改局部位置参数
                if (auto geom2_accessor = geometry_storage.acquire_read(m2.geometry_handle)) {
                    const auto& geom2 = *geom2_accessor;
                    
                    if (auto transform2_accessor = transform_storage.acquire_write(geom2.transform_handle)) {
                        auto& transform2 = *transform2_accessor;
                        
                        // 直接修改局部位置参数
                        transform2.position.x += total_offset.x;
                        transform2.position.y += total_offset.y;
                        transform2.position.z += total_offset.z;
                    }
                }
            }
        }
    }
}

void MechanicsSystem::shutdown() {
    CFW_LOG_NOTICE("MechanicsSystem: Shutting down...");
}
}  // namespace Corona::Systems