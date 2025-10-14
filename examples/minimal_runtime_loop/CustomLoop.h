#pragma once

#include "src/script/python/public/PythonAPI.h"
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
        python_api_.runPythonScript();
    }

    void on_tick() override {
        python_api_.runPythonScript();
    }

    void on_shutdown() override {}

private:
    PythonAPI python_api_;

};
