#include <corona/events/engine_events.h>
#include <corona/events/kinematics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/kinematics/kinematics_system.h>
#include <ktm/ktm.h>

namespace Corona::Systems {
bool KinematicsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("KinematicsSystem: Initializing...");
    return true;
}

void KinematicsSystem::update() {
}

void KinematicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("KinematicsSystem: Shutting down...");
}
}  // namespace Corona::Systems