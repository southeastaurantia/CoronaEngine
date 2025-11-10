#pragma once

#include <corona/events/geometry_system_events.h>
#include <corona/kernel/event/i_event_bus.h>
#include <corona/kernel/event/i_event_stream.h>
#include <corona/kernel/system/system_base.h>

#include <memory>

namespace Corona::Systems {

/**
 * @brief 几何系统 (Geometry System)
 *
 * 负责几何数据管理、空间变换、层次结构和包围盒计算。
 * 运行在独立线程，以 60 FPS 更新几何状态。
 */
class GeometrySystem : public Kernel::SystemBase {
   public:
    GeometrySystem() {
        set_target_fps(60);  // 几何系统运行在 60 FPS
    }

    ~GeometrySystem() override = default;

    // ========================================
    // ISystem 接口实现
    // ========================================

    std::string_view get_name() const override {
        return "Geometry";
    }

    int get_priority() const override {
        return 85;  // 高优先级，在动画系统之后
    }

    /**
     * @brief 初始化几何系统
     * @param ctx 系统上下文
     * @return 初始化成功返回 true
     */
    bool initialize(Kernel::ISystemContext* ctx) override;

    /**
     * @brief 每帧更新几何
     *
     * 在独立线程中调用，更新几何变换和空间结构
     */
    void update() override;

    /**
     * @brief 关闭几何系统
     *
     * 清理所有几何资源
     */
    void shutdown() override;

   private:
    // 几何系统私有成员
};

}  // namespace Corona::Systems
