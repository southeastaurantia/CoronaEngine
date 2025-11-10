#include <corona/events/acoustics_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/acoustics/acoustics_system.h>

namespace Corona::Systems {

bool AcousticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("AcousticsSystem: Initializing...");
    return true;
}

void AcousticsSystem::update() {
}

void AcousticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AcousticsSystem: Shutting down...");
}

}  // namespace Corona::Systems
