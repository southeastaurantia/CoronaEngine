#pragma once

#include <PythonAPI.h>
#include <PythonBridge.h>
#include <engine/RuntimeLoop.h>

// 继承 RuntimeLoop，但不改其实现；在构造时设置每帧回调
// 这样每个 example 可以定义一个自己的 CustomLoop，注入不同的帧逻辑
class CustomLoop : public RuntimeLoop {
public:
    explicit CustomLoop(Corona::Engine& engine)
        : RuntimeLoop(engine) {

    }

protected:
    void on_initialize() override {
        Corona::Engine::instance().add_queue("MainThread", std::make_unique<Corona::SafeCommandQueue>());
        // 注册主线程 sender，用于把消息转发到 Python
        Corona::PythonBridge::set_sender([this](const std::string& msg){ this->send_message(msg); });
    }

    void on_tick() override {
        auto& main_queue = Corona::Engine::instance().get_queue("MainThread");

        int spun = 0;
        while (spun < 100 && !main_queue.empty()) {
            if (!main_queue.try_execute()) {
                continue;
            }
            ++spun;
        }

        python_api_.runPythonScript();
    }

    void on_shutdown() override {
        // 清理 sender，避免关闭阶段还有转发
        Corona::PythonBridge::clear_sender();
    }

private:
    PythonAPI python_api_;
    void send_message(const std::string &message) const {
        python_api_.sendMessage(message);
    }

};
