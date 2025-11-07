#include <corona/events/engine_events.h>
#include <corona/events/mechanics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/mechanics_system.h>

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

}

void MechanicsSystem::update_physics() {

}

void MechanicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("MechanicsSystem: Shutting down...");
}

}  // namespace Corona::Systems
