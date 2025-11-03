#include <corona/systems/animation_system.h>
#include <corona/shared_data_hub.h>

namespace Corona::Systems {

bool AnimationSystem::initialize(Kernel::ISystemContext* ctx) {
    // TODO: 初始化动画系统资源
    // - 注册动画组件
    // - 初始化动画状态管理器
    // - 订阅相关事件

    SharedDataHub::instance().demo_data_storage();

    return true;
}

void AnimationSystem::update() {
    // TODO: 每帧更新动画
    // - 遍历所有动画实体
    // - 根据 delta_time() 更新动画进度
    // - 应用骨骼变换
    // - 触发动画事件
}

void AnimationSystem::shutdown() {
    // TODO: 清理动画系统资源
    // - 停止所有动画
    // - 释放动画数据
    // - 取消事件订阅
}

}  // namespace Corona::Systems