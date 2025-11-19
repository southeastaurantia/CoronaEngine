#include "corona/engine.h"

#include <corona/events/engine_events.h>
#include <corona/resource_manager/resource_manager.h>
#include <corona/script/python/python_api.h>
#include <corona/systems/acoustics/acoustics_system.h>
#include <corona/systems/display/display_system.h>
#include <corona/systems/geometry/geometry_system.h>
#include <corona/systems/kinematics/kinematics_system.h>
#include <corona/systems/mechanics/mechanics_system.h>
#include <corona/systems/optics/optics_system.h>
#include <corona/systems/script/script_system.h>

#include <chrono>
#include <memory>
#include <thread>

#include "corona/resource_manager/model.h"
#include "corona/resource_manager/text_file.h"

namespace Corona {

// ============================================================================
// 构造与析构
// ============================================================================

Engine::Engine()
    : kernel_(Kernel::KernelContext::instance()),
      initialized_(false),
      running_(false),
      exit_requested_(false),
      frame_number_(0),
      last_frame_time_(0.0f) {
}

Engine::~Engine() {
    if (running_.load()) {
        shutdown();
    }
}

// ============================================================================
// 生命周期管理
// ============================================================================

bool Engine::initialize() {
    if (initialized_.load()) {
        return true;
    }

    // 1. 初始化 KernelContext
    if (!kernel_.initialize()) {
        return false;
    }

    auto* logger = kernel_.logger();
    logger->info("====================================");
    logger->info("CoronaEngine Initializing...");
    logger->info("====================================");

    // 2. 注册核心系统
    if (!register_systems()) {
        logger->error("Failed to register systems");
        return false;
    }

    auto& resource_manager = ResourceManager::instance();
    resource_manager.register_loader(std::make_shared<TextFileLoader>());
    resource_manager.register_loader(std::make_shared<ModelLoader>());

    // 3. 初始化所有系统
    auto* sys_mgr = kernel_.system_manager();
    if (!sys_mgr || !sys_mgr->initialize_all()) {
        logger->error("Failed to initialize systems");
        return false;
    }

    initialized_.store(true);

    logger->info("====================================");
    logger->info("CoronaEngine Initialized Successfully");
    logger->info("====================================");

    return true;
}

void Engine::run() {
    Script::Python::PythonAPI python_api;
    auto* logger = kernel_.logger();
    if (!initialized_.load()) {
        logger->error("Cannot run engine: not initialized");
        return;
    }

    if (running_.load()) {
        logger->warning("Engine is already running");
        return;
    }

    running_.store(true);
    exit_requested_.store(false);

    logger->info("====================================");
    logger->info("CoronaEngine Starting Main Loop");
    logger->info("====================================");

    // 启动所有系统线程
    auto* sys_mgr = kernel_.system_manager();
    if (sys_mgr) {
        sys_mgr->start_all();
    }

    // 主循环
    auto last_time = std::chrono::high_resolution_clock::now();
    constexpr auto target_frame_duration = std::chrono::microseconds(8333);  // 120 FPS

    while (!exit_requested_.load()) {
        auto frame_start_time = std::chrono::high_resolution_clock::now();

        // 计算帧时间
        std::chrono::duration<float> delta_duration = frame_start_time - last_time;
        last_frame_time_ = delta_duration.count();
        last_time = frame_start_time;

        // 执行一帧
#ifdef CORONA_ENABLE_PYTHON_API
        python_api.runPythonScript();
#endif
        tick();

        // 帧号递增
        frame_number_++;

        // 帧率控制（120 FPS）
        auto frame_end_time = std::chrono::high_resolution_clock::now();
        auto frame_elapsed = frame_end_time - frame_start_time;

        // 计算剩余时间并 sleep
        if (frame_elapsed < target_frame_duration) {
            auto sleep_duration = target_frame_duration - frame_elapsed;
            std::this_thread::sleep_for(sleep_duration);
        }
    }

    logger->info("====================================");
    logger->info("CoronaEngine Main Loop Exited");
    logger->info("====================================");

    running_.store(false);
}

void Engine::request_exit() {
    exit_requested_.store(true);

    auto* logger = kernel_.logger();
    logger->info("Engine exit requested");
}

void Engine::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    auto* sys_mgr = kernel_.system_manager();
    if (sys_mgr) {
        sys_mgr->stop_all();
        sys_mgr->shutdown_all();
    }

    auto* logger = kernel_.logger();
    logger->info("====================================");
    logger->info("CoronaEngine Shutting Down...");
    logger->info("====================================");

    // 关闭内核（SystemManager 的析构函数会自动调用 shutdown_all() 和 stop_all()）
    // 注意：kernel_.shutdown() 会重置 logger，所以之后不能再使用 logger 指针
    kernel_.shutdown();

    initialized_.store(false);

    // 不要在 kernel_.shutdown() 之后使用 logger，因为它已经被释放
}

// ============================================================================
// 状态查询
// ============================================================================

bool Engine::is_initialized() const {
    return initialized_.load();
}

bool Engine::is_running() const {
    return running_.load();
}

// ============================================================================
// 系统访问
// ============================================================================

Kernel::KernelContext& Engine::kernel() {
    return kernel_;
}

Kernel::ISystemManager* Engine::system_manager() {
    return kernel_.system_manager();
}

Kernel::ILogger* Engine::logger() {
    return kernel_.logger();
}

Kernel::IEventBus* Engine::event_bus() {
    return kernel_.event_bus();
}

Kernel::IEventBusStream* Engine::event_stream() {
    return kernel_.event_stream();
}

// ============================================================================
// 内部方法
// ============================================================================

bool Engine::register_systems() {
    auto* sys_mgr = kernel_.system_manager();
    if (!sys_mgr) {
        return false;
    }

    auto* logger = kernel_.logger();

    // 注册核心系统（按优先级自动排序）
    // Display(100) > Optics(90) > Geometry(85) > Animation(80) > Mechanics(75) > Acoustics(70)

    logger->info("Registering core systems...");

    // Display System - 最高优先级
    sys_mgr->register_system(std::make_shared<Systems::DisplaySystem>());
    logger->info("  - DisplaySystem registered (priority 100)");

    // Optics System (光学系统)
    sys_mgr->register_system(std::make_shared<Systems::OpticsSystem>());
    logger->info("  - OpticsSystem registered (priority 90)");

    // Geometry System (几何系统)
    sys_mgr->register_system(std::make_shared<Systems::GeometrySystem>());
    logger->info("  - GeometrySystem registered (priority 85)");

    // Animation System (动画系统)
    sys_mgr->register_system(std::make_shared<Systems::KinematicsSystem>());
    logger->info("  - AnimationSystem registered (priority 80)");

    // Mechanics System (力学系统)
    sys_mgr->register_system(std::make_shared<Systems::MechanicsSystem>());
    logger->info("  - MechanicsSystem registered (priority 75)");

    // Acoustics System (声学系统)
    sys_mgr->register_system(std::make_shared<Systems::AcousticsSystem>());
    logger->info("  - AcousticsSystem registered (priority 70)");

    sys_mgr->register_system(std::make_shared<Systems::ScriptSystem>());
    logger->info("  - ScriptSystem registered (priority 60)");

    logger->info("All core systems registered");

    return true;
}

void Engine::tick() {
    auto* logger = kernel_.logger();

    // 4. 更新系统上下文的帧信息
    // 系统通过 SystemBase 的 delta_time() 和 frame_number() 访问帧信息

    // 5. 同步所有系统（可选）
    // 系统在各自的线程中运行，主循环可以在这里进行跨系统的同步

    // 6. 收集性能统计
}

}  // namespace Corona