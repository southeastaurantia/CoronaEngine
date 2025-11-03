#include <corona/systems/acoustics_system.h>

#include <corona/events/acoustics_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>

namespace Corona::Systems {

bool AcousticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("AcousticsSystem: Initializing event demo");

    // 【DEMO】订阅跨线程事件（使用 EventStream）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        // event_stream->subscribe<Events::EngineToAcousticsDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("AcousticsSystem: Received EngineToAcousticsDemoEvent, value={}", event.value);
        // });
        logger->info("AcousticsSystem: EventStream subscriptions ready (demo)");
    }

    // 【DEMO】订阅系统内部事件（使用 EventBus）
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        // event_bus->subscribe<Events::AcousticsSystemDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("AcousticsSystem: Internal event, sound_count={}", event.sound_count);
        // });
        logger->info("AcousticsSystem: EventBus subscriptions ready (demo)");
    }

    return true;
}

void AcousticsSystem::update() {
    static int frame_count = 0;
    frame_count++;

    // 【DEMO】每 90 帧发送一次跨线程事件到引擎
    if (frame_count % 90 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            // Events::AcousticsToEngineDemoEvent event{delta_time()};
            // event_stream->publish(event);
            context()->logger()->info("AcousticsSystem: Would send AcousticsToEngineDemoEvent (demo)");
        }
    }

    // 【DEMO】每 180 帧发送一次系统内部事件
    if (frame_count % 180 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            // Events::AcousticsSystemDemoEvent event{5};
            // event_bus->publish(event);
            context()->logger()->info("AcousticsSystem: Would publish internal event (demo)");
        }
    }
}

void AcousticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AcousticsSystem: Shutting down event demo");
}

}  // namespace Corona::Systems
