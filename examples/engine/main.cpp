#include <corona/engine.h>
#include <corona/systems/script/python_api.h>

#include <csignal>
#include <iostream>
#include <thread>

// 全局引擎实例指针，用于信号处理
static Corona::Engine* g_engine = nullptr;

/**
 * @brief 信号处理函数
 *
 * 捕获 Ctrl+C 等中断信号，优雅退出引擎
 */
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Signal] Interrupt received, requesting engine shutdown..." << std::endl;
        if (g_engine) {
            g_engine->request_exit();
        }
    }
}

/**
 * @brief CoronaEngine 主程序
 *
 * 功能：
 * 1. 初始化 CoronaEngine
 * 2. 注册信号处理器
 * 3. 启动主循环
 * 4. 优雅关闭引擎
 */
int main(int argc, char* argv[]) {
    std::cout << std::endl;
    std::cout << "    +==================================================================+" << std::endl;
    std::cout << "    |                                                                  |" << std::endl;
    std::cout << "    |                      CoronaEngine v0.5.0                         |" << std::endl;
    std::cout << "    |                                                                  |" << std::endl;
    std::cout << "    |              A Modern Game Engine Framework                      |" << std::endl;
    std::cout << "    |                                                                  |" << std::endl;
    std::cout << "    +==================================================================+" << std::endl;
    std::cout << std::endl;

    // 创建引擎实例
    Corona::Engine engine;

    Corona::Kernel::CoronaLogger::set_log_level(Corona::Kernel::LogLevel::debug);

    g_engine = &engine;

    // 注册信号处理器
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // ========================================
    // 1. 初始化引擎
    // ========================================
    std::cout << "[Main] Initializing engine..." << std::endl;

    if (!engine.initialize()) {
        std::cerr << "[Main] ERROR: Failed to initialize engine!" << std::endl;
        return -1;
    }

    std::cout << "[Main] Engine initialized successfully" << std::endl;
    std::cout << std::endl;

    // ========================================
    // 2. 启动主循环（在独立线程）
    // ========================================
    std::cout << "[Main] Starting engine main loop..." << std::endl;
    std::cout << "[Main] Press Ctrl+C to exit" << std::endl;
    std::cout << std::endl;

    // 在独立线程运行引擎主循环
    std::thread engine_thread([&engine]() {
        engine.run();
    });

    // ========================================
    // 3. 等待用户输入或信号中断
    // ========================================

    // 等待引擎线程结束（用户按 Ctrl+C 或调用 request_exit()）
    engine_thread.join();

    // ========================================
    // 4. 关闭引擎
    // ========================================
    std::cout << std::endl;
    std::cout << "[Main] Shutting down engine..." << std::endl;

    engine.shutdown();

    std::cout << "[Main] Engine shutdown complete" << std::endl;
    std::cout << std::endl;
    std::cout << "+==================================================================+" << std::endl;
    std::cout << "|                Thank you for using CoronaEngine!                 |" << std::endl;
    std::cout << "+==================================================================+" << std::endl;

    g_engine = nullptr;
    return 0;
}
