#include <corona/events/engine_events.h>
#include <corona/events/geometry_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/geometry/geometry_system.h>

namespace Corona::Systems {

bool GeometrySystem::initialize(Kernel::ISystemContext* ctx) {
    CFW_LOG_NOTICE("GeometrySystem: Initializing...");
    return true;
}

void GeometrySystem::update() {
}

void GeometrySystem::shutdown() {
    CFW_LOG_NOTICE("GeometrySystem: Shutting down...");
}

}  // namespace Corona::Systems
