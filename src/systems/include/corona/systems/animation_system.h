#pragma once

#include <corona/kernel/system/system_base.h>

namespace Corona::Systems {

/**
 * @brief 动画系统
 *
 * 负责管理和更新所有动画状态，包括骨骼动画、变换动画等。
 * 运行在独立线程，以 60 FPS 更新动画逻辑。
 */
class AnimationSystem : public Kernel::SystemBase {
   public:
    AnimationSystem() {
        set_target_fps(60);  // 动画系统运行在 60 FPS
    }

    ~AnimationSystem() override = default;

    // ========================================
    // ISystem 接口实现
    // ========================================

    std::string_view get_name() const override {
        return "Animation";
    }

    int get_priority() const override {
        return 80;  // 高优先级，在渲染前更新
    }

    /**
     * @brief 初始化动画系统
     * @param ctx 系统上下文
     * @return 初始化成功返回 true
     */
    bool initialize(Kernel::ISystemContext* ctx) override;

    /**
     * @brief 每帧更新动画
     *
     * 在独立线程中调用，根据 delta_time() 更新动画状态
     */
    void update() override;

    /**
     * @brief 关闭动画系统
     *
     * 清理所有动画资源
     */
    void shutdown() override;

   private:
    // 动画系统内部状态（待实现）
};

}  // namespace Corona::Systems
