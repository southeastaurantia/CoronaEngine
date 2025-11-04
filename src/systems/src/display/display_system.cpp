#include <corona/events/display_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/display_system.h>

namespace Corona::Systems {

bool DisplaySystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("DisplaySystem: Initializing...");

    // 【订阅系统内部事件】使用 EventBus
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        logger->info("DisplaySystem: EventBus subscriptions ready");
    }

    return true;
}

void DisplaySystem::update() {
}

void DisplaySystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("DisplaySystem: Shutting down...");

}

}  // namespace Corona::Systems
