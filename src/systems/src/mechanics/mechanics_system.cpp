#include <corona/systems/mechanics_system.h>

#include <corona/events/engine_events.h>
#include <corona/events/mechanics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>

namespace Corona::Systems {

bool MechanicsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("MechanicsSystem: Initializing...");
    return true;
}

void MechanicsSystem::update() {
}

void MechanicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("MechanicsSystem: Shutting down...");
}

}  // namespace Corona::Systems
