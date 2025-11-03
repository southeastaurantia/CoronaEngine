#include <corona/systems/animation_system.h>

#include <corona/events/animation_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/shared_data_hub.h>

namespace Corona::Systems {

bool AnimationSystem::initialize(Kernel::ISystemContext* ctx) {
    // 初始化动画系统资源
    SharedDataHub::instance().demo_data_storage();

    auto* logger = ctx->logger();
    logger->info("AnimationSystem: Initializing event demo");

    // 【DEMO】订阅跨线程事件（使用 EventStream）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        // 订阅来自引擎的消息
        // event_stream->subscribe<Events::EngineToAnimationDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("AnimationSystem: Received EngineToAnimationDemoEvent, value={}", event.value);
        // });
        
        // 订阅帧开始事件
        // event_stream->subscribe<Events::FrameBeginEvent>([this](const auto& event) {
        //     // 可以在这里同步主线程的帧信息
        // });
        
        logger->info("AnimationSystem: EventStream subscriptions ready (demo)");
    }

    // 【DEMO】订阅系统内部事件（使用 EventBus）
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        // event_bus->subscribe<Events::AnimationSystemDemoEvent>([this](const auto& event) {
        //     context()->logger()->info("AnimationSystem: Internal event, count={}", event.animation_count);
        // });
        
        logger->info("AnimationSystem: EventBus subscriptions ready (demo)");
    }

    return true;
}

void AnimationSystem::update() {
    // 每帧更新动画逻辑
    static int frame_count = 0;
    frame_count++;

    // 【DEMO】每 60 帧发送一次跨线程事件到引擎
    if (frame_count % 60 == 0) {
        auto* event_stream = context()->event_stream();
        if (event_stream) {
            // Events::AnimationToEngineDemoEvent event{delta_time()};
            // event_stream->publish(event);
            context()->logger()->info("AnimationSystem: Would send AnimationToEngineDemoEvent (demo)");
        }
    }

    // 【DEMO】每 120 帧发送一次系统内部事件
    if (frame_count % 120 == 0) {
        auto* event_bus = context()->event_bus();
        if (event_bus) {
            // Events::AnimationSystemDemoEvent event{42};
            // event_bus->publish(event);
            context()->logger()->info("AnimationSystem: Would publish internal event (demo)");
        }
    }
}

void AnimationSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AnimationSystem: Shutting down event demo");
    
    // 清理动画系统资源
    // 取消事件订阅会在析构时自动完成
}

}  // namespace Corona::Systems