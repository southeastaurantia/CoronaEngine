#include <corona/systems/mechanics_system.h>

#include <corona/events/engine_events.h>
#include <corona/events/mechanics_system_events.h>
#include <corona/kernel/core/i_logger.h>

namespace Corona::Systems {

bool MechanicsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("MechanicsSystem: Initializing event demo");

    // 【DEMO】订阅跨线程事件（使用 EventStream）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        // event_stream->subscribe<Events::EngineToMechanicsDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("MechanicsSystem: Received EngineToMechanicsDemoEvent, value={}", event.value);
        // });
        logger->info("MechanicsSystem: EventStream subscriptions ready (demo)");
    }

    // 【DEMO】订阅系统内部事件（使用 EventBus）
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        // event_bus->subscribe<Events::MechanicsSystemDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("MechanicsSystem: Internal event, physics_step={}", event.physics_step_count);
        // });
        logger->info("MechanicsSystem: EventBus subscriptions ready (demo)");
    }

    return true;
}

void MechanicsSystem::update() {
    static int frame_count = 0;
    frame_count++;

    // 【DEMO】每 100 帧发送一次跨线程事件到引擎
    if (frame_count % 100 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            // Events::MechanicsToEngineDemoEvent event{delta_time()};
            // event_stream->publish(event);
            context()->logger()->info("MechanicsSystem: Would send MechanicsToEngineDemoEvent (demo)");
        }
    }

    // 【DEMO】每 200 帧发送一次系统内部事件
    if (frame_count % 200 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            // Events::MechanicsSystemDemoEvent event{10};
            // event_bus->publish(event);
            context()->logger()->info("MechanicsSystem: Would publish internal event (demo)");
        }
    }
}

void MechanicsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("MechanicsSystem: Shutting down event demo");
}

}  // namespace Corona::Systems
