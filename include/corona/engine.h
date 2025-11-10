#pragma once

#include <corona/kernel/core/kernel_context.h>

#include <atomic>

namespace Corona {

/**
 * @brief CoronaEngine 主引擎类
 *
 * Engine 是 CoronaEngine 的核心管理类，负责：
 * - 初始化 CoronaFramework KernelContext
 * - 注册和管理核心系统（Animation, Audio, Display, Rendering）
 * - 管理主循环和引擎生命周期
 * - 提供系统访问接口
 *
 * 使用示例：
 * @code
 * Corona::Engine engine;
 * if (!engine.initialize()) {
 *     return -1;
 * }
 *
 * engine.run();  // 主循环
 * engine.shutdown();
 * @endcode
 */
class Engine {
   public:
    /**
     * @brief 构造函数
     */
    Engine();

    /**
     * @brief 析构函数
     *
     * 确保引擎正确关闭
     */
    ~Engine();

    // 禁止拷贝和移动
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    // ========================================
    // 生命周期管理
    // ========================================

    /**
     * @brief 初始化引擎
     *
     * 初始化顺序：
     * 1. 初始化 KernelContext（日志、事件、VFS 等）
     * 2. 注册核心系统（Display -> Rendering -> Animation -> Audio）
     * 3. 初始化所有系统
     *
     * @return 初始化成功返回 true，失败返回 false
     */
    bool initialize();

    /**
     * @brief 运行主循环
     *
     * 启动所有系统并进入主循环。
     * 主循环负责：
     * - 更新帧时间
     * - 同步系统
     * - 处理引擎级事件
     *
     * 调用 request_exit() 可退出主循环
     */
    void run();

    /**
     * @brief 请求退出引擎
     *
     * 设置退出标志，主循环将在当前帧结束后退出
     */
    void request_exit();

    /**
     * @brief 关闭引擎
     *
     * 关闭顺序：
     * 1. 停止所有系统线程
     * 2. 关闭所有系统
     * 3. 关闭 KernelContext
     */
    void shutdown();

    // ========================================
    // 状态查询
    // ========================================

    /**
     * @brief 检查引擎是否已初始化
     * @return 已初始化返回 true
     */
    bool is_initialized() const;

    /**
     * @brief 检查引擎是否正在运行
     * @return 正在运行返回 true
     */
    bool is_running() const;

    // ========================================
    // 系统访问
    // ========================================

    /**
     * @brief 获取 KernelContext
     * @return KernelContext 引用
     */
    Kernel::KernelContext& kernel();

    /**
     * @brief 获取系统管理器
     * @return 系统管理器指针，未初始化返回 nullptr
     */
    Kernel::ISystemManager* system_manager();

    /**
     * @brief 获取日志系统
     * @return 日志系统指针，未初始化返回 nullptr
     */
    Kernel::ILogger* logger();

    /**
     * @brief 获取事件总线
     * @return 事件总线指针，未初始化返回 nullptr
     */
    Kernel::IEventBus* event_bus();

    /**
     * @brief 获取事件流
     * @return 事件流指针，未初始化返回 nullptr
     */
    Kernel::IEventBusStream* event_stream();

   private:
    // ========================================
    // 内部方法
    // ========================================

    /**
     * @brief 注册所有核心系统
     * @return 注册成功返回 true
     */
    bool register_systems();

    /**
     * @brief 主循环的单次迭代
     *
     * 更新帧时间、同步系统、处理事件
     */
    void tick();

    // ========================================
    // 成员变量
    // ========================================

    Kernel::KernelContext& kernel_;     ///< KernelContext 引用
    std::atomic<bool> initialized_;     ///< 初始化标志
    std::atomic<bool> running_;         ///< 运行标志
    std::atomic<bool> exit_requested_;  ///< 退出请求标志

    uint64_t frame_number_;  ///< 当前帧号
    float last_frame_time_;  ///< 上一帧时间（秒）
};

}  // namespace Corona