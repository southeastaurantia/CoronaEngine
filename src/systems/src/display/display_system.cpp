#include <corona/systems/display_system.h>

#include <corona/events/display_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>

namespace Corona::Systems {

bool DisplaySystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("DisplaySystem: Initializing event demo");

    // 【订阅跨线程事件】使用 EventStream
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        engine_stream_ = event_stream->get_stream<Events::EngineToDisplayDemoEvent>();
        engine_sub_ = engine_stream_->subscribe();
        logger->info("DisplaySystem: EventStream subscriptions ready");
    }

    return true;
}

void DisplaySystem::update() {
    // 处理来自引擎的跨线程事件
    while (auto event = engine_sub_.try_pop()) {
        context()->logger()->info("DisplaySystem: Received EngineToDisplayDemoEvent, delta_time=" + std::to_string(event->delta_time));
    }

    static int frame_count = 0;
    frame_count++;

    // 每 50 帧发送一次跨线程事件到引擎
    if (frame_count % 50 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            Events::DisplayToEngineDemoEvent event{delta_time()};
            event_stream->get_stream<Events::DisplayToEngineDemoEvent>()->publish(event);
            context()->logger()->info("DisplaySystem: Published DisplayToEngineDemoEvent");
        }
    }

    // 每 100 帧发送一次系统内部事件（EventBus 仅在当前系统线程使用）
    if (frame_count % 100 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            Events::DisplaySystemDemoEvent event{100};
            event_bus->publish(event);
            context()->logger()->info("DisplaySystem: Published internal EventBus event");
        }
    }
}

void DisplaySystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("DisplaySystem: Shutting down event demo");
    
    // 关闭 EventStream 订阅
    engine_sub_.close();
}

}  // namespace Corona::Systems
