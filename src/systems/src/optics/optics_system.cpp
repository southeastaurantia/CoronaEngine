#include <corona/systems/optics_system.h>

#include <corona/events/engine_events.h>
#include <corona/events/optics_system_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>

namespace Corona::Systems {

bool OpticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("OpticsSystem: Initializing event demo");

    // 【订阅跨线程事件】使用 EventStream
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        engine_stream_ = event_stream->get_stream<Events::EngineToOpticsDemoEvent>();
        engine_sub_ = engine_stream_->subscribe();
        logger->info("OpticsSystem: EventStream subscriptions ready");
    }

    return true;
}

void OpticsSystem::update() {
    // 处理来自引擎的跨线程事件
    while (auto event = engine_sub_.try_pop()) {
        context()->logger()->info("OpticsSystem: Received EngineToOpticsDemoEvent, delta_time=" + std::to_string(event->delta_time));
    }

    static int frame_count = 0;
    frame_count++;

    // 每 75 帧发送一次跨线程事件到引擎
    if (frame_count % 75 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            Events::OpticsToEngineDemoEvent event{delta_time()};
            event_stream->get_stream<Events::OpticsToEngineDemoEvent>()->publish(event);
            context()->logger()->info("OpticsSystem: Published OpticsToEngineDemoEvent");
        }
    }

    // 每 150 帧发送一次系统内部事件（EventBus 仅在当前系统线程使用）
    if (frame_count % 150 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            Events::OpticsSystemDemoEvent event{123};
            event_bus->publish(event);
            context()->logger()->info("OpticsSystem: Published internal EventBus event");
        }
    }
}

void OpticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("OpticsSystem: Shutting down event demo");
    
    // 关闭 EventStream 订阅
    engine_sub_.close();
}

}  // namespace Corona::Systems
