#include <corona/events/engine_events.h>
#include <corona/events/geometry_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/geometry_system.h>

namespace Corona::Systems {

bool GeometrySystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("GeometrySystem: Initializing...");
    return true;
}

void GeometrySystem::update() {
}

void GeometrySystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("GeometrySystem: Shutting down...");
}

}  // namespace Corona::Systems
