#pragma once

#include <corona/kernel/system/system_base.h>

namespace Corona::Systems {

/**
 * @brief 渲染系统
 *
 * 负责场景渲染、GPU 资源管理和渲染管线控制。
 * 运行在独立线程，以 60 FPS 渲染场景。
 */
class RenderingSystem : public Kernel::SystemBase {
   public:
    RenderingSystem() {
        set_target_fps(60);  // 渲染系统运行在 60 FPS
    }

    ~RenderingSystem() override = default;

    // ========================================
    // ISystem 接口实现
    // ========================================

    std::string_view get_name() const override {
        return "Rendering";
    }

    int get_priority() const override {
        return 90;  // 高优先级，在显示系统之后初始化
    }

    /**
     * @brief 初始化渲染系统
     * @param ctx 系统上下文
     * @return 初始化成功返回 true
     */
    bool initialize(Kernel::ISystemContext* ctx) override;

    /**
     * @brief 每帧渲染
     *
     * 在独立线程中调用，执行场景渲染
     */
    void update() override;

    /**
     * @brief 关闭渲染系统
     *
     * 清理所有 GPU 资源和渲染管线
     */
    void shutdown() override;

   private:
    // 渲染系统内部状态（待实现）
};

}  // namespace Corona::Systems