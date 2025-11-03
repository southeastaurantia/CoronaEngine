#include <corona/systems/geometry_system.h>

#include <corona/events/engine_events.h>
#include <corona/events/geometry_system_events.h>
#include <corona/kernel/core/i_logger.h>

namespace Corona::Systems {

bool GeometrySystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("GeometrySystem: Initializing event demo");

    // 【DEMO】订阅跨线程事件（使用 EventStream）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        // event_stream->subscribe<Events::EngineToGeometryDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("GeometrySystem: Received EngineToGeometryDemoEvent, value={}", event.value);
        // });
        logger->info("GeometrySystem: EventStream subscriptions ready (demo)");
    }

    // 【DEMO】订阅系统内部事件（使用 EventBus）
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        // event_bus->subscribe<Events::GeometrySystemDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("GeometrySystem: Internal event, mesh_count={}", event.mesh_count);
        // });
        logger->info("GeometrySystem: EventBus subscriptions ready (demo)");
    }

    return true;
}

void GeometrySystem::update() {
    static int frame_count = 0;
    frame_count++;

    // 【DEMO】每 80 帧发送一次跨线程事件到引擎
    if (frame_count % 80 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            // Events::GeometryToEngineDemoEvent event{delta_time()};
            // event_stream->publish(event);
            context()->logger()->info("GeometrySystem: Would send GeometryToEngineDemoEvent (demo)");
        }
    }

    // 【DEMO】每 160 帧发送一次系统内部事件
    if (frame_count % 160 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            // Events::GeometrySystemDemoEvent event{256};
            // event_bus->publish(event);
            context()->logger()->info("GeometrySystem: Would publish internal event (demo)");
        }
    }
}

void GeometrySystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("GeometrySystem: Shutting down event demo");
}

}  // namespace Corona::Systems
