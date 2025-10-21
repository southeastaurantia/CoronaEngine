# CoronaEngine 开发者指南

本文总结了扩展或贡献 CoronaEngine 时的关键流程、约定与实践建议。

## 核心概念
- **Engine 单例**：通过 `Engine::instance()` 获取引擎（参考 `include/corona/core/Engine.h`）。它负责系统注册、命令队列管理、数据缓存枢纽与事件总线。请勿额外构造引擎实例。
- **系统（System）**：每个系统继承自 `include/corona/interfaces/ThreadedSystem.h`，运行在独立工作线程，源码位于 `src/systems/<module>`，公共头文件在 `include/corona/systems`。为系统注册队列时调用 `Engine::add_queue(name(), std::make_unique<SafeCommandQueue>())`。
- **数据流**：场景与演员操作通过 `CoronaEngineAPI`（`include/corona/api/CoronaEngineAPI.h`）执行。该 API 操作静态 `entt::registry`，并使用 `Engine::events<T>().publish(topic, payload)` 发布事件；渲染与动画系统订阅这些主题保持同步。
- **线程安全**：资源回调若在其他线程执行，务必将对引擎状态的操作重新派发到对应系统的队列（例如 `engine.get_queue(name()).enqueue(...)`），避免跨线程直接操作局部状态。

## 使用 SafeCommandQueue
- 入队 Lambda 时尽量捕获轻量上下文，避免阻塞 MPMC 队列。
- `SafeCommandQueue::enqueue` 支持函数对象、成员函数指针、`shared_ptr` 等多种形式，请根据场景选择最轻量方案。
- 系统通常限制每帧处理的任务数（如 RenderingSystem 设为 128），保持类似上限以免占用过多帧时间。

## SafeDataCache 用法
- `include/corona/threading/SafeDataCache.h` 中定义，使用 `uint64_t` 作为键、`shared_ptr` 存储数据。
- `modify()` 会针对单个 ID 加锁，请在其中安全修改数据。
- `safe_loop_foreach()` 若 `try_lock` 失败会重试该 ID，确保循环能容忍重新加锁，避免热点数据饥饿。

## 事件与变换规则
- 变换更新中，零向量被视为“无变化”。若需重置变换，请发送极小值或显式标记说明意图。
- 在 `onStop()` 中使用保存的订阅句柄取消订阅，避免事件监听泄漏。

## 资源管理
- 通过 `Engine::resources()` 获取 `CoronaResource` 管理器。
- 使用 `ResourceId::from(...)` 包装资源标识，优先调用 `load_once_async(...)` 让回调排入系统队列。
- 新增资源类型时遵循 `CoronaResource` 现有模式，保持运行时依赖复制机制可用。

## Python 集成
- Python 嵌入接口位于 `include/corona/script`。使用 `PythonBridge::set_sender` 注册主线程分发器；`AnimationSystem::send_collision_event` 演示了如何将事件排入主线程队列供 Python 消费。
- 嵌入式解释器的配置由 `misc/cmake/corona_python.cmake` 提供；调整脚本或热更新流程时，更新 `PythonHotfix` 与 `EngineScripts` 列表。

## 日志
- 使用 `corona_logger`，并通过 `CE_LOG_*` 宏输出。请在 `main` 中调用 `Engine::init` 之前配置日志，以确保全局日志一致。

## 构建与工具
- 使用 `CMakePresets.json` 中的预设配置构建（如 `cmake --preset ninja-msvc`）。
- `code-format.ps1` 会用 clang-format 处理已暂存的 C++ 变更；在 CI 或提交前可运行 `./code-format.ps1 -Check`。
- 通过 `-DBUILD_CORONA_EDITOR=ON` 启用编辑器资源复制，对应逻辑定义在 `corona_editor.cmake`。
- `examples/minimal_runtime_loop` 可作为系统/事件改动的快速冒烟测试（需开启 `CORONA_BUILD_EXAMPLES`）。

## 贡献流程
1. **分支**：从 `main` 创建功能分支，保持修改聚焦。
2. **格式化**：提交前运行 `./code-format.ps1`，保持代码风格统一。
3. **测试**：在提交 PR 前运行最小运行时示例或自定义冒烟测试；新增功能时同步补充单元或集成测试。
4. **文档**：流程变化后更新 `docs/CMAKE_GUIDE.md`、本指南或相关注释，确保文档准确。
5. **Pull Request**：提供摘要、行为变更及测试说明。修改关键模块（渲染、线程、资源管理）时标记子系统负责人。

## 调试提示
- 使用 Visual Studio 或 RenderDoc 调试渲染系统；渲染系统依赖场景事件快照，请确认事件是否正确发布。
- 若遇到线程问题，可在关键的 `SafeCommandQueue` 操作处加入日志，追踪任务是否被丢弃或堆积。
- 孤立 Python 集成问题时，可设置环境变量 `CORONA_PY_LEAKSAFE=1`，在热重载期间禁用 DECREF 降低噪音。

## 新增系统步骤
1. 在 `src/systems/<module>` 下定义继承 `ThreadedSystem` 的新系统。
2. 在运行时循环（`engine/RuntimeLoop.cpp`）中注册该系统，并确保通过 `Engine::add_queue` 配备专属队列。
3. 参考现有系统（如 `RenderingSystem`）的订阅与清理模式，正确管理事件和队列。
4. 在 `include/corona/systems` 中添加公共接口头文件，必要时通过 `CoronaEngineAPI` 暴露。
5. 补充测试或示例（可复用最小运行时示例）以验证新系统。

## 风格提示
- 代码注释与日志建议统一使用英文，以维持一致性。
- 注释应解释意图而非重复代码行为。
- 在合适场景使用现代 C++20 特性（结构化绑定、智能指针、`std::optional` 等）提升可读性。

请遵循 `include/corona/core/Engine.h` 描述的总体架构，以及 `src/` 下各模块的分层设计。若有疑问，可参照类似系统或咨询 CoronaEngine 维护者。