#include <corona/systems/acoustics_system.h>

#include <corona/events/acoustics_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>

namespace Corona::Systems {

bool AcousticsSystem::initialize(Kernel::ISystemContext* ctx) {
    auto* logger = ctx->logger();
    logger->info("AcousticsSystem: Initializing event demo");

    // 【订阅跨线程事件】使用 EventStream（可以跨线程调用）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        // 订阅来自引擎的消息
        engine_stream_ = event_stream->get_stream<Events::EngineToAcousticsDemoEvent>();
        engine_sub_ = engine_stream_->subscribe();
        
        logger->info("AcousticsSystem: EventStream subscriptions ready");
    }

    return true;
}

void AcousticsSystem::update() {
    static int frame_count = 0;
    frame_count++;

    // 【处理跨线程事件】非阻塞拉取
    if (engine_sub_.is_valid()) {
        while (auto event = engine_sub_.try_pop()) {
            std::string msg = "AcousticsSystem: Received EngineToAcousticsDemoEvent, delta_time=" +
                            std::to_string(event->delta_time);
            context()->logger()->info(msg);
        }
    }

    // 【每 90 帧发送跨线程事件到引擎】
    if (frame_count % 90 == 0 && engine_stream_) {
        // 获取引擎流并发布
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            auto engine_response_stream = event_stream->get_stream<Events::AcousticsToEngineDemoEvent>();
            engine_response_stream->publish(Events::AcousticsToEngineDemoEvent{delta_time()});
            context()->logger()->info("AcousticsSystem: Sent AcousticsToEngineDemoEvent");
        }
    }

    // 【每 180 帧发送系统内部事件】
    if (frame_count % 180 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            event_bus->publish(Events::AcousticsSystemDemoEvent{5});
            context()->logger()->info("AcousticsSystem: Published internal event");
        }
    }
}

void AcousticsSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AcousticsSystem: Shutting down event demo");
    
    // 关闭 EventStream 订阅
    engine_sub_.close();
}

}  // namespace Corona::Systems
