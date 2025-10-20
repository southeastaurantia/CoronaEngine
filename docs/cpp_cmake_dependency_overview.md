# CoronaEngine C++/CMake 依赖关系分析

## 项目模块概览
- `src/`：核心静态库 `CoronaEngine` 的源代码，按 `core`、`systems`、`thread`、`script`、`utils` 划分模块；公开头正逐步迁移到仓库根部的 `include/corona/**` 树。
- `engine/`：运行时可执行程序 `Corona_runtime`，封装默认系统注册与主循环控制。
- `examples/`：示例（默认启用 `minimal_runtime_loop`），演示如何复用 `RuntimeLoop` 并挂接 Python 热更新逻辑。
- `misc/cmake/`：自定义 CMake 工具脚本（模块收集、选项、Python/第三方依赖配置、运行时依赖打包等）。
- `third_party/`：预置的嵌入式 Python 3.13.7，以及部分外部库源仓库（通过 FetchContent 同步）。

## CMake 目标依赖总览
| 目标 | 类型 | 公共头文件根目录 | 直接依赖（target_link_libraries） | 说明 |
| --- | --- | --- | --- | --- |
| `CoronaUtils` (`corona::utils`) | INTERFACE | `include/corona/utils` | 无 | 提供跨平台编译器/平台探测宏等基础设施。
| `CoronaThread` (`corona::thread`) | INTERFACE | `include/corona/threading` | `Corona::Logger`, `cabbage::concurrent` | 暴露线程安全容器、队列与事件总线；依赖日志与 CabbageConcurrent 队列实现。
| `CoronaSystemInterface` (`corona::system::interface`) | INTERFACE | `include/corona/interfaces` | `Corona::Logger` | 声明系统基类 `ISystem` 与线程化调度骨架 `ThreadedSystem`。
| `CoronaSystemAnimation` (`corona::system::animation`) | STATIC | `include/corona/systems` | `corona::system::interface`, `corona::core`, `CoronaResource::Resource` | 动画线程系统，编译期依赖核心与资源模块；产生与核心的双向耦合。
| `CoronaSystemAudio` (`corona::system::audio`) | STATIC | `include/corona/systems` | `corona::system::interface`, `corona::core` | 音频线程系统，目前示例级实现。
| `CoronaSystemDisplay` (`corona::system::display`) | STATIC | `include/corona/systems` | `corona::system::interface`, `corona::core` | 显示线程系统，结构与 Audio 相同。
| `CoronaSystemRendering` (`corona::system::rendering`) | STATIC | `include/corona/systems` | `corona::system::interface`, `corona::core`, `CoronaResource::Resource`, `CabbageHardware` | 渲染线程系统，依赖底层硬件抽象。
| `CoronaScriptPython` (`corona::script::python`) | STATIC | `include/corona/script` | `corona::core`, `Corona::Logger`, `python313` | 嵌入式 Python 支持，暴露 Python API/热更新；需核心模块与日志。
| `CoronaCore` (`corona::core`) | STATIC | `include/corona/core` | `corona::script::python`, `corona::system::animation`, `corona::system::rendering`, `corona::system::display`, `corona::system::audio`, `corona::thread`, `corona::utils`, `ktm`, `CoronaResource::Resource` | 引擎单例、事件/缓存中心，集中链接所有系统与资源；存在与各系统的环状依赖。
| `CoronaEngine` (`corona::engine`) | STATIC(聚合) | 继承子模块公共目录 | `_corona_modules` 列表中的全部子目标 | 入口静态库，转发所有模块依赖与编译特性。
| `Corona_runtime` | EXECUTABLE | `engine/` | `CoronaEngine`, `EnTT::EnTT` | 主程序，负责初始化日志、引擎及默认 `RuntimeLoop`。

### 其他 CMake 脚本要点
- `corona_collect_module.cmake`：按模块自动收集 `include`/`public` 头与 `src`/`private` 源，生成标准变量供 `add_library` 使用。
- `corona_options.cmake`：集中声明 `BUILD_CORONA_RUNTIME`、`CORONA_BUILD_EXAMPLES`、`BUILD_CORONA_EDITOR` 等开关。
- `corona_python.cmake`：强制使用内置 Python 3.13.7，配置依赖校验 `check_python_deps`。
- `corona_third_party.cmake`：通过 `FetchContent` 拉取 Assimp、stb、EnTT、CoronaLogger、CabbageHardware、CabbageConcurrent、CoronaResource 等外部仓库。
- `corona_runtime_deps.cmake` / `corona_editor.cmake`：处理运行时 DLL 与编辑器资源拷贝（在库或可执行目标调用）。

## 第三方依赖来源
- **CoronaLogger**：提供 `corona_logger.h` 与日志初始化/宏。
- **CoronaResource**：暴露 `ResourceManager.h`、`Model.h`、`Shader.h` 等资源系统接口。
- **CabbageConcurrent**：提供 `MPMCQueue`、`ConcurrentHashMap` 等无锁结构，支撑安全队列/事件总线。
- **CabbageHardware**：渲染后端，包含 `CabbageDisplayer.h`、`Pipeline/*` 类等 GPU 抽象。
- **EnTT**：ECS 框架，运行时主循环与 `CoronaEngineAPI` 均依赖。
- **ktm**：数学库，为场景、动画和渲染模块提供矩阵/向量运算。
- **Python 3.13.7**：嵌入式解释器，Python API、热更新与桥接模块需其头/库。

## 头文件 include 关系梳理
### 核心模块（`src/core`）
- `Engine.h`：通过 `<ResourceManager.h>`、`<corona_logger.h>` 依赖资源与日志库，引用 `<corona/interfaces/ISystem.h>`、`<corona/threading/SafeCommandQueue.h>`、`<corona/threading/SafeDataCache.h>`、`<corona/threading/EventBus.h>` 将系统接口与线程工具集中到核心入口。
- `Engine.cpp`：结合核心服务适配器与资源头（`"Model.h"`、`"Shader.h"`），负责单例初始化与服务注册；系统注册已迁移至运行时装配流程。
- `CoronaEngineAPI.h`：依赖 `<ktm/ktm.h>` 与 EnTT registry；后者通过 `../../../build/_deps/entt-src/src/entt/entity/registry.hpp` 直接指向构建目录，需关注路径稳定性与可移植性。
- `include/corona/core/components/*.h` 与 `include/corona/core/events/ActorEvents.h`：定义 ECS 组件与事件载荷，依赖 `ktm` 向量类型但未引入额外外部库。

### 线程与并发模块（`include/corona/threading`）
- `SafeCommandQueue.h`：基于 `Cabbage::Concurrent::MPMCQueue` 实现命令队列，提供函数、成员函数、`shared_ptr` 等多种 `enqueue` 重载。
- `SafeDataCache.h`：组合 `ConcurrentHashMap` 与逐项互斥锁，支持并行 `insert/erase/modify` 与循环遍历，日志输出统一使用 `corona_logger`。
- `EventBus.h`：模板化发布/订阅总线，Topic -> 订阅者队列映射基于 `ConcurrentHashMap`，每个订阅者拥有独立的 `MPMCQueue`。

### 系统接口与系统模块（`src/systems`）
#### 接口子模块
- `ISystem.h`：定义抽象生命周期接口。
- `ThreadedSystem.h`：继承 `ISystem`，通过 `<thread>` 驱动专用线程并按目标 FPS 控制循环，同时依赖 `corona_logger` 输出运行日志。

#### 动画系统（`animation`）
- `AnimationSystem.h`：包含 `"ThreadedSystem.h"` 与外部 `"AnimationState.h"`，前置声明 `Animation`、`Bone`、`Model`。
- `AnimationSystem.cpp`：引入 `"Animation.h"`、`"Bone.h"`、`"Model.h"`（来自 `CoronaResource`）、`<corona/core/Engine.h>`（核心模块）、`<corona/script/PythonBridge.h>`（脚本模块）；内部频繁访问 `Engine::instance()` 的资源管理器、命令队列与缓存。

#### 渲染系统（`rendering`）
- `RenderingSystem.h`：依赖 `"ThreadedSystem.h"`、CabbageHardware 的 `<CabbageDisplayer.h>`、`<Pipeline/ComputePipeline.h>`、`<Pipeline/RasterizerPipeline.h>`，以及线程模块的 `<corona/threading/EventBus.h>`、本模块的 `<corona/systems/rendering/SceneEvents.h>`、核心事件 `<corona/core/events/ActorEvents.h>`。
- `SceneEvents.h`：定义摄像机更新、太阳光、显示表面、场景移除事件，使用 `ktm::fvec3`，现位于 `include/corona/systems/rendering`。
- `RenderingSystem.cpp`：包含 `<corona/core/Engine.h>`、`"Mesh.h"`、`"Model.h"`、`"Shader.h"` 等头，`onTick` 中消费命令队列、事件队列并访问 `SafeDataCache<Model>`。

#### 音频与显示系统（`audio`、`display`）
- 公共头均只依赖 `"ThreadedSystem.h"`。
- 实现文件包含 `<corona/core/Engine.h>` 与 `<filesystem>`，示例性地使用资源管理器异步加载 shader 并回投结果至系统队列。

### 脚本与 Python 模块（`src/script/python`）
- `EngineScripts.h`：包含 `<Python.h>` 与 `<corona/api/CoronaEngineAPI.h>`，定义 Python 包装类型以暴露 `Scene`/`Actor` 方法。
- `PythonAPI.h`：依赖 STL、`<corona/script/PythonHotfix.h>`、`<corona/script/EngineScripts.h>`，管理热更新周期、模块列表与消息发送；内部维护 `PyConfig`、热重载策略。
- `PythonHotfix.h`：纯 STL + `<Python.h>`，实现脚本扫描、依赖图与重载逻辑。
- `PythonBridge.h`/`.cpp`：只依赖标准库，提供线程安全的消息派发器，供系统线程向 Python 主线程转发事件。

### 运行时与示例
- `engine/main.cpp`：包含 `<corona/core/Engine.h>`、`<corona_logger.h>` 与 `"RuntimeLoop.h"`，负责初始化日志/引擎并驱动运行时循环。
- `engine/RuntimeLoop.h|cpp`：通过 `<corona/systems/AnimationSystem.h>`、`<corona/systems/RenderingSystem.h>`、`<corona/systems/AudioSystem.h>`、`<corona/systems/DisplaySystem.h>`、`<corona/api/CoronaEngineAPI.h>`、`<entt/entt.hpp>` 连接各系统；根据 `CoronaEngineAPI` tag 启停线程系统。
- `examples/minimal_runtime_loop`：`main.cpp` 与 `CustomLoop.h` 同时包含四大系统与 `<corona/script/PythonAPI.h>`、`<corona/script/PythonBridge.h>`，示范创建额外命令队列（`"MainThread"`）并在自定义循环中轮询。

## 交叉依赖与潜在风险
- **核心与系统的循环链接**：`CoronaCore` 链接 `corona::system::*`，而每个系统又链接回 `corona::core`，静态库在 MSVC 下可工作，但增加链接排序敏感性，对拆分/动态库化不友好。
- **源码级强耦合**：`Engine.cpp` 必须包含全部系统头才能注册系统，而系统实现又大量调用 `Engine::instance()`，层次难以解耦并拉长编译时间。
- **第三方头路径依赖构建目录**：`CoronaEngineAPI.h` 直接引用 `build/_deps` 下的 EnTT 头文件，若改用安装版 EnTT 或清理构建目录会失败，建议改用 `<entt/entt.hpp>`。
- **外部资源头显式缺席仓库**：`AnimationState.h`、`Model.h`、`Shader.h` 等来自 `CoronaResource`，仓库本体缺失这些文件，必须依赖 FetchContent 拉取成功。
- **命令队列命名冲突风险**：各系统在构造时基于 `name()` 注册命令队列，示例又添加 `MainThread` 队列，缺乏统一命名或冲突检查机制。
- **注释编码不一致**：如 `Engine.cpp` 内部出现乱码注释，提示历史文件编码与当前编译器设置不一致。

## 建议
- **拆解循环依赖**：引擎核心仅依赖系统接口与线程模块，具体系统注册移至运行时装配或独立工厂，消除静态库互相链接。
- **规范第三方 include**：通过 `target_link_libraries` 暴露的 include 目录引用 EnTT、CoronaResource，移除对 `build/_deps` 的硬编码。
- **定义系统访问协议**：通过消息/配置对象向系统注入依赖，避免在实现中直接调用全局单例。
- **补充模块文档**：在各 `public` 目录或头文件顶部记录来源（自研/第三方）、依赖前置条件，降低外部接入门槛。
- **统一队列注册**：考虑在 `Engine` 中增加命令队列命名规范或辅助注册接口，防止重复 key。