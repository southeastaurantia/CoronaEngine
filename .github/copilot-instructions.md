# CoronaEngine Copilot 指南

## 快速概览
- 核心引擎位于 `Src/Core`（`Engine`、`Systems`、`Thread`）与 `Src/Resource`；`Src/Script/Python` 提供内嵌脚本入口。
- 通用组件拆到 `Utility/Logger` 与 `Utility/ResourceManager`，第三方依赖由 `Misc/cmake/CoronaThirdParty.cmake` 通过 FetchContent 统一拉取（assimp、EnTT、GLFW 等）。
- 示例程序在 `Examples/`，`interactive_rendering` 展示了系统注册、缓存写入和 GLFW 输出的完整流程，是交互逻辑的最佳参考。

## 引擎生命周期与线程模型
- 入口流程：`Engine::Instance().Init(cfg)` → `RegisterSystem<T>()` → `StartSystems()`；`Shutdown()` 会依次停止系统并清理资源。
- 所有运行时系统继承 `ThreadedSystem`（默认 120 FPS），`onTick()` 在专用线程中调用；如需自定义帧率使用 `SetTargetFps`。
- 与系统交互必须通过 `SafeCommandQueue`：注册系统时由其构造函数调用 `Engine::AddQueue(name(), std::make_unique<SafeCommandQueue>())`，业务线程用 `Engine::GetQueue(name()).enqueue(...)` 投递任务。
- `CoronaEngineAPI`（`Src/Core/CoronaEngineAPI.*`）封装了 actor / scene 生命周期：内部使用 `DataId::Next()`、缓存写入和系统队列，供外部模块（Python、客户端）调用。

## 数据缓存与资源加载
- 使用 `Engine::Cache<T>()` 获取 `SafeDataCache<T>`；插入前请调用 `DataId::Next()` 分配 id，修改数据需传 callback（见 `SafeDataCache::modify`）。
- 遍历场景/模型时使用 `safe_loop_foreach`，该函数处理 try-lock 重试；回调应短小，避免阻塞系统线程。
- `ResourceManager` (`Utility/ResourceManager`) 默认缓存资源，`load()` 返回共享指针，`loadOnce()`/`loadAsync()` 适合一次性读取或后台加载；自定义 loader 参考 `Examples/resource_management/resource_management.cpp`。

## Utility 模块约定
- `Utility/Logger` 只通过 `<Log.h>` 对外暴露，内部隐藏 spdlog；请在入口（例如 `Engine::Init`）调用 `Logger::Init`，业务侧统一使用 `CE_LOG_*` 宏，不直接触达第三方日志 API。
- `Utility/ResourceManager` 的 Loader 需在引擎初始化阶段注册一次；`ResourceId::ComputeUid` 会标准化路径（小写与 `/`），新增资源类型时复用该生成逻辑并放在 `ResourceTypes` 架构下。
- `Utility` 下新增可复用工具模块时沿用现有结构：源码放在模块根目录，公共头集中在 `include/` 并由根 `CMakeLists.txt` 暴露；保持线程安全或跨系统通用能力的实现放在这里，供 `Src/Core` 与示例共享。

## 渲染与动画协作
- `RenderingSystem`（`Src/Core/Engine/Systems/RenderingSystem.*`）在 `onTick()` 消费命令队列后调用 `updateEngine()`：遍历 `Scene` 与 `Model` 缓存、刷新 g-buffer、写回 `HardwareImage`，并将最终输出绑定到 `Scene::displayer`。
- `AnimationSystem` 通过 `state_cache_keys_` 与 `model_cache_keys_` 管理骨骼动画和碰撞；`updateAnimationState` 会填充 `AnimationState::bones` 并刷新 `Model::bonesMatrixBuffer`。
- 向渲染或动画系统注册资源时务必调用 `WatchScene` / `WatchModel`（见 `CoronaEngineAPI::Scene`、`CoronaEngineAPI::Actor` 构造函数），销毁前对称调用 `Unwatch*` 以避免悬挂引用。
- 需要新增系统时，沿用 `ThreadedSystem` + `SafeCommandQueue` 模式，并在 `Engine::StartSystems()` 前完成 `RegisterSystem`。

## Python 与编辑器集成
- `Misc/cmake/CoronaPython.cmake` 会优先检测系统 Python≥`CORONA_PYTHON_MIN_VERSION`，否则回退到 `Env/Python-3.13.7`；配置阶段默认执行 `Misc/pytools/check_pip_modules.py` 校验 requirements。
- `Script/Python/PythonAPI.*` 将 `Editor/CoronaEditor/Backend` 打包为嵌入式模块 `CoronaEngine`，内置热更新（`PythonHotfix`）与 `PyInit_CoronaEngineEmbedded` 类型注册。
- 构建编辑器资源需开启 `-DBUILD_CORONA_EDITOR=ON`，随后 `corona_install_corona_editor` 调用 `Misc/pytools/editor_copy_and_build.py` 使用 `Env/node-v22.19.0` 运行 `npm install && npm run build`；错误只发出警告但不会终止构建。

## 构建与运行工作流
- 首次配置：`cmake --preset ninja-mc`（PowerShell）；常用构建 `cmake --build --preset ninja-debug --target Corona_interactive_rendering`，其他配置参见 `CMakePresets.json`。
- 关键选项：`CORONA_BUILD_EXAMPLES` 控制示例，`BUILD_CORONA_EDITOR` 控制编辑器资源，`BUILD_SHARED_LIBS` 默认为 OFF；所有开关定义在 `Misc/cmake/CoronaOptions.cmake`。
- 运行示例前确保 `Misc/pytools/check_pip_modules.py` 通过（如需手动复查可执行 `cmake --build --preset ninja-debug --target check_python_deps`）。
- 生成的可执行与依赖 DLL 会被 `corona_install_runtime_deps` 复制到目标目录；如添加新依赖，请更新 `Misc/cmake/CoronaRuntimeDeps.cmake`。

## Examples 开发现约
- 通过 `Examples/CMakeLists.txt` 中的 `corona_add_example` 增加示例：指定 `NAME`、`SOURCES`，可选 `COPY_ASSETS` 控制是否拷贝 `Examples/assets/`。
- 每个子目录默认启用 `BUILD_EXAMPLE_<NAME>` 开关，可在配置时逐项关闭；示例目标自动链接 `CoronaEngine` 与 `glfw`。
- 示例运行目录通常位于 `$<TARGET_FILE_DIR>`，`VS_DEBUGGER_WORKING_DIRECTORY` 已指向可执行所在目录，资产访问请基于 `std::filesystem::current_path()/assets`。

## 代码约定与调试
- 日志统一走 `CE_LOG_*` 宏（`Utility/Logger/include/Log.h`），默认级别在 `Engine::Init` 时配置；自定义级别使用 `Logger::SetLevel`。
- 资源 id 与缓存 key 使用 `uint64_t`，避免手工复用旧 id；销毁对象时必须 `erase` 缓存并让系统取消关注。
- 渲染资源（`HardwareImage`、`HardwareBuffer` 等）由 `CabbageHardware` 提供；在 CPU 线程创建后通过命令队列与 GPU 线程同步。
- 资产路径默认基于运行目录的 `assets/`（示例中使用 `std::filesystem::current_path()/assets`），跨平台时请保持正斜杠并复用 `ResourceId::ComputeUid` 的规范化逻辑。
- 命名遵循 `.clang-tidy` 的 google-readability 配置：类/结构体/接口/枚举使用 `CamelCase`，自由函数与局部变量 `snake_case`，成员变量 `snake_case_`（自动追加 `_`），常量与枚举值使用 `kCamelCase` 前缀；main 风格入口可放宽检查。

## 排除目录
- `build/`
- `.cache/`
