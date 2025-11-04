#include <corona/events/animation_system_events.h>
#include <corona/events/engine_events.h>
#include <corona/kernel/core/i_logger.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/shared_data_hub.h>
#include <corona/systems/animation_system.h>

namespace Corona::Systems {

bool AnimationSystem::initialize(Kernel::ISystemContext* ctx) {
    // 初始化动画系统资源
    SharedDataHub::instance().demo_data_storage();

    auto* logger = ctx->logger();
    logger->info("AnimationSystem: Initializing...");
    return true;
}

void AnimationSystem::update() {
}

void AnimationSystem::shutdown() {
    auto* logger = context()->logger();
    logger->info("AnimationSystem: Shutting down...");
}

}  // namespace Corona::Systems