#include <corona/systems/optics_system.h>

#include <corona/events/engine_events.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/i_logger.h>

namespace Corona::Systems {

bool OpticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("OpticsSystem: Initializing event demo");

    // 【DEMO】订阅跨线程事件（使用 EventStream）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        // event_stream->subscribe<Events::EngineToOpticsDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("OpticsSystem: Received EngineToOpticsDemoEvent, value={}", event.value);
        // });
        logger->info("OpticsSystem: EventStream subscriptions ready (demo)");
    }

    // 【DEMO】订阅系统内部事件（使用 EventBus）
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        // event_bus->subscribe<Events::OpticsSystemDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("OpticsSystem: Internal event, render_pass={}", event.render_pass_id);
        // });
        logger->info("OpticsSystem: EventBus subscriptions ready (demo)");
    }

    return true;
}

void OpticsSystem::update() {
    static int frame_count = 0;
    frame_count++;

    // 【DEMO】每 75 帧发送一次跨线程事件到引擎
    if (frame_count % 75 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            // Events::OpticsToEngineDemoEvent event{delta_time()};
            // event_stream->publish(event);
            context()->logger()->info("OpticsSystem: Would send OpticsToEngineDemoEvent (demo)");
        }
    }

    // 【DEMO】每 150 帧发送一次系统内部事件
    if (frame_count % 150 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            // Events::OpticsSystemDemoEvent event{123};
            // event_bus->publish(event);
            context()->logger()->info("OpticsSystem: Would publish internal event (demo)");
        }
    }
}

void OpticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("OpticsSystem: Shutting down event demo");
}

}  // namespace Corona::Systems
