#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/systems/optics_system.h>

namespace Corona::Systems {

bool OpticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("OpticsSystem: Initializing...");

    // 【订阅系统内部事件】使用 EventBus
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        surface_changed_sub_id_ = event_bus->subscribe<Events::DisplaySurfaceChangedEvent>(
            [this, logger](const Events::DisplaySurfaceChangedEvent& event) {
                if (logger) {
                    logger->info("OpticsSystem: Received DisplaySurfaceChangedEvent, new surface: " +
                                 std::to_string(reinterpret_cast<uintptr_t>(event.surface)));
                }
                // 在这里可以处理 surface 变化，例如更新渲染目标等
                this->surface_handle_ = event.surface;
            });
        logger->info("OpticsSystem: Subscribed to DisplaySurfaceChangedEvent");
    }

    return true;
}

void OpticsSystem::update() {

}

void OpticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("OpticsSystem: Shutting down...");

    // 取消 EventBus 订阅
    auto* event_bus = context()->event_bus();
    if (event_bus) {
        if (surface_changed_sub_id_ != 0) {
            event_bus->unsubscribe(surface_changed_sub_id_);
        }
    }

}

}  // namespace Corona::Systems
