#include <corona/systems/display_system.h>

#include <corona/events/display_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>

namespace Corona::Systems {

bool DisplaySystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("DisplaySystem: Initializing event demo");

    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        logger->info("DisplaySystem: EventStream subscriptions ready (demo)");
    }

    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        logger->info("DisplaySystem: EventBus subscriptions ready (demo)");
    }

    return true;
}

void DisplaySystem::update() {
    static int frame_count = 0;
    frame_count++;

    if (frame_count % 50 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            context()->logger()->info("DisplaySystem: Would send DisplayToEngineDemoEvent (demo)");
        }
    }

    if (frame_count % 100 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            context()->logger()->info("DisplaySystem: Would publish internal event (demo)");
        }
    }
}

void DisplaySystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("DisplaySystem: Shutting down event demo");
}

}  // namespace Corona::Systems
