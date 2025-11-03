#pragma once

#include <corona/events/acoustics_system_events.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/kernel/system/system_base.h>

#include <memory>

namespace Corona::Systems {

/**
 * @brief 声学系统 (Acoustics System)
 *
 * 负责管理声音播放、混音、3D 声学和音频处理。
 * 运行在独立线程，以 60 FPS 处理声学逻辑。
 */
class AcousticsSystem : public Kernel::SystemBase {
   public:
    AcousticsSystem() {
        set_target_fps(60);  // 声学系统运行在 60 FPS
    }

    ~AcousticsSystem() override = default;

    // ========================================
    // ISystem 接口实现
    // ========================================

    std::string_view get_name() const override {
        return "Acoustics";
    }

    int get_priority() const override {
        return 70;  // 中等优先级
    }

    /**
     * @brief 初始化声学系统
     * @param ctx 系统上下文
     * @return 初始化成功返回 true
     */
    bool initialize(Kernel::ISystemContext* ctx) override;

    /**
     * @brief 每帧更新声学
     *
     * 在独立线程中调用，处理声音播放和混音
     */
    void update() override;

    /**
     * @brief 关闭声学系统
     *
     * 停止所有声音播放并清理资源
     */
    void shutdown() override;

   private:
    // 跨线程事件订阅（EventStream）
    std::shared_ptr<Kernel::EventStream<Events::EngineToAcousticsDemoEvent>> engine_stream_;
    Kernel::EventSubscription<Events::EngineToAcousticsDemoEvent> engine_sub_;
};

}  // namespace Corona::Systems
