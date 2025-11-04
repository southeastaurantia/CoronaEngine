#pragma once

#include <corona/events/optics_system_events.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/kernel/system/system_base.h>

#include <memory>

namespace Corona::Systems {

/**
 * @brief 光学系统 (Optics System)
 *
 * 负责场景光学渲染、光线追踪、GPU 资源管理和渲染管线控制。
 * 运行在独立线程，以 60 FPS 渲染场景。
 */
class OpticsSystem : public Kernel::SystemBase {
   public:
    OpticsSystem() {
        set_target_fps(60);  // 光学系统运行在 60 FPS
    }

    ~OpticsSystem() override = default;

    // ========================================
    // ISystem 接口实现
    // ========================================

    std::string_view get_name() const override {
        return "Optics";
    }

    int get_priority() const override {
        return 90;  // 高优先级，在显示系统之后初始化
    }

    /**
     * @brief 初始化光学系统
     * @param ctx 系统上下文
     * @return 初始化成功返回 true
     */
    bool initialize(Kernel::ISystemContext* ctx) override;

    /**
     * @brief 每帧渲染
     *
     * 在独立线程中调用，执行场景光学渲染
     */
    void update() override;

    /**
     * @brief 关闭光学系统
     *
     * 清理所有 GPU 资源和渲染管线
     */
    void shutdown() override;

   private:
    // TODO: 添加光学系统私有成员
    std::shared_ptr<Kernel::EventStream<Events::EngineToOpticsDemoEvent>> engine_stream_;
    Kernel::EventSubscription<Events::EngineToOpticsDemoEvent> engine_sub_;

    // 系统内部事件订阅（EventBus）
    Kernel::EventId internal_event_id_ = 0;
};

}  // namespace Corona::Systems
