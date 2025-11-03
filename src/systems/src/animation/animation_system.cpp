#include <corona/systems/animation_system.h>

#include <corona/events/animation_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>

namespace Corona::Systems {

bool AnimationSystem::initialize(Kernel::ISystemContext* ctx) {
    // 初始化动画系统资源
    SharedDataHub::instance().demo_data_storage();

    auto* logger = ctx->logger();
    logger->info("AnimationSystem: Initializing event demo");

    // 【订阅跨线程事件】使用 EventStream
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        engine_stream_ = event_stream->get_stream<Events::EngineToAnimationDemoEvent>();
        engine_sub_ = engine_stream_->subscribe();
        logger->info("AnimationSystem: EventStream subscriptions ready");
    }

    // 【订阅系统内部事件】使用 EventBus
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        internal_event_id_ = event_bus->subscribe<Events::AnimationSystemDemoEvent>(
            [logger](const Events::AnimationSystemDemoEvent& event) {
                if (logger) {
                    logger->info("AnimationSystem: Received internal event, demo_value=" + std::to_string(event.demo_value));
                }
            });
        logger->info("AnimationSystem: EventBus subscriptions ready");
    }

    return true;
}

void AnimationSystem::update() {
    // 处理来自引擎的跨线程事件
    while (auto event = engine_sub_.try_pop()) {
        context()->logger()->info("AnimationSystem: Received EngineToAnimationDemoEvent, delta_time=" + std::to_string(event->delta_time));
    }

    // 每帧更新动画逻辑
    static int frame_count = 0;
    frame_count++;

    // 每 60 帧发送一次跨线程事件到引擎
    if (frame_count % 60 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            Events::AnimationToEngineDemoEvent event{delta_time()};
            event_stream->get_stream<Events::AnimationToEngineDemoEvent>()->publish(event);
            context()->logger()->info("AnimationSystem: Published AnimationToEngineDemoEvent");
        }
    }

    // 每 120 帧发送一次系统内部事件（EventBus 仅在当前系统线程使用）
    if (frame_count % 120 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            Events::AnimationSystemDemoEvent event{42};
            event_bus->publish(event);
            context()->logger()->info("AnimationSystem: Published internal EventBus event");
        }
    }
}

void AnimationSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AnimationSystem: Shutting down event demo");
    
    // 取消 EventBus 订阅
    auto* event_bus = context()->event_bus();
    if (event_bus && internal_event_id_ != 0) {
        event_bus->unsubscribe(internal_event_id_);
    }
    
    // 关闭 EventStream 订阅
    engine_sub_.close();
    
    // 清理动画系统资源
}

}  // namespace Corona::Systems