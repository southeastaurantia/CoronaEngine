# Engine::instance() 使用统计

> 数据来源：`git grep -n "Engine::instance"`（2025-10-18）

| 模块/文件 | 调用次数 | 主要用途 |
| --- | --- | --- |
| `src/systems/animation/private/AnimationSystem.cpp` | 8 | 命令队列注册、资源加载、访问数据缓存、发送事件到主线程 |
| `src/systems/audio/private/AudioSystem.cpp` | 4 | 命令队列注册、异步资源加载、命令队列轮询 |
| `src/systems/display/private/DisplaySystem.cpp` | 4 | 同音频系统，用于队列注册与轮询 |
| `src/systems/rendering/private/RenderingSystem.cpp` | 25 | 队列注册、事件订阅/退订、资源加载、模型缓存访问、系统互访 |
| `src/core/private/CoronaEngineAPI.cpp` | 10 | 通过 API 对外暴露事件发布接口 |
| `examples/minimal_runtime_loop/*.cpp|h` | 11 | 示例程序中获取系统、队列和引擎实例 |
| `engine/main.cpp` | 1 | 引擎初始化 |

> 结论：渲染系统对 `Engine::instance()` 的依赖最重（25 次），其次是动画系统。后续重构优先为渲染系统设计 `SystemContext` 注入路径，并为动画系统补充资源/队列服务的接口。
