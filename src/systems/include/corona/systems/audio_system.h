#pragma once

#include <corona/kernel/system/system_base.h>

namespace Corona::Systems {

/**
 * @brief 音频系统
 *
 * 负责管理音频播放、混音和 3D 音效处理。
 * 运行在独立线程，以 60 FPS 处理音频逻辑。
 */
class AudioSystem : public Kernel::SystemBase {
   public:
    AudioSystem() {
        set_target_fps(60);  // 音频系统运行在 60 FPS
    }

    ~AudioSystem() override = default;

    // ========================================
    // ISystem 接口实现
    // ========================================

    std::string_view get_name() const override {
        return "Audio";
    }

    int get_priority() const override {
        return 70;  // 中等优先级
    }

    /**
     * @brief 初始化音频系统
     * @param ctx 系统上下文
     * @return 初始化成功返回 true
     */
    bool initialize(Kernel::ISystemContext* ctx) override;

    /**
     * @brief 每帧更新音频
     *
     * 在独立线程中调用，处理音频播放和混音
     */
    void update() override;

    /**
     * @brief 关闭音频系统
     *
     * 停止所有音频播放并清理资源
     */
    void shutdown() override;

   private:
    // 音频系统内部状态（待实现）
};

}  // namespace Corona::Systems