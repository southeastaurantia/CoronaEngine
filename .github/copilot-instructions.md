# CoronaEngine AI Handoff

## 🧭 架构总览
- 核心单例 `Src/Core/Engine/Engine.{h,cpp}` 负责初始化日志、资源管理器，并维护系统注册表、命令队列和 `SafeDataCache<T>` 数据仓。
- 系统以 `ThreadedSystem` 为基类（`Core/Engine/ThreadedSystem.h`），默认独立线程以 120 FPS 调度；典型实现见 `Systems/AnimationSystem.cpp`。
- 渲染、动画、音频、显示等子系统通过 `Engine::RegisterSystem<T>()` 注册，并在 `StartSystems/StopSystems` 生命周期中启动/终止自身线程。

## 🔄 线程与数据共享模式
- 每个系统在构造函数里调用 `Engine::AddQueue(name(), ...)` 注册 `SafeCommandQueue`；`onTick()` 中循环 `try_execute()` 最多 100 条命令，避免饿死。
- 线程间共享对象通过 `Engine::Cache<T>()` 返回的 `SafeDataCache<T>` 完成：`insert`/`erase` 管理生命周期，`modify` 在持锁状态下安全更新，`safe_loop_foreach` 支持无阻塞遍历。
- 生成跨系统数据 ID 请使用 `DataId::Next()`（`Engine.h`），示例可参考 `Examples/interactive_rendering/interactive_rendering.cpp`。

## 📦 资源加载管线
- `Utility/ResourceManager` 维护 `ResourceId{type,path,uid}` → `std::shared_ptr<IResource>` 映射，基于 oneTBB 并发容器和 `task_group` 实现缓存与异步加载。
- 默认在 `Engine::Init()` 注册了 `ModelLoader` 与 `ShaderLoader`，前者依赖 Assimp+stb (`Src/Resource/Model.cpp`)，后者期待 `path/shaders/test.{vert,frag,comp}.glsl` 结构。
- 若需扩展，派生 `IResourceLoader` 并注册：参见 `Examples/resource_management/resource_management.cpp` 中的 `MyConfigLoader`。

## 🖼 渲染与实体交互
- 模型、场景等运行时数据通过缓存与系统命令队列协作：`RenderingSystem::WatchModel` / `WatchScene` 在队列线程中处理，触发方式见 `CoronaEngineAPI.cpp` 和 `Examples/interactive_rendering`。
- 动画系统利用 `AnimationState` 缓存骨骼矩阵，`AnimationSystem::onTick()` 中迭代 `state_cache_keys_` 推进时间，并更新 GPU 缓冲。
- `CoronaEngineAPI::Actor` / `Scene` 暴露给 Python，内部自动装配缓存与系统回调，适合脚本或嵌入式调用。

## 🐍 Python 集成
- Python 嵌入入口在 `Src/Script/Python/PythonAPI.cpp`：`ensureInitialized()` 使用 `CORONA_PYTHON_*` 宏配置解释器搜索路径，并将 `CoronaEngine` 模块注册到 PyImport。
- 热重载由 `PythonHotfix` 监控 `Editor/CoronaEditor/Backend`，触发 `ReloadPythonFile()` 后重新导入模块；示例主循环见 `Examples/python_scripting/python_scripting.cpp`。
- CMake 配置阶段会运行 `Misc/pytools/check_pip_modules.py` 校验 `Misc/pytools/requirements.txt`，必要时可手动执行 `cmake --build --preset ninja-debug --target check_python_deps`。

## 🛠 构建与运行流程
- 首次配置：在仓库根目录执行 `cmake --preset ninja-mc`（或 `--preset vs2022`）。常见开关集中在 `Misc/cmake/CoronaOptions.cmake`：`CORONA_BUILD_EXAMPLES`、`BUILD_CORONA_EDITOR`、`BUILD_SHARED_LIBS` 等，可通过 `cmake --preset ninja-mc -D...=...` 覆盖。
- 构建：`cmake --build --preset ninja-debug` 等多配置预设。运行示例需保证工作目录含 `Examples/assets`，常用产物如 `build/bin/examples/Corona_interactive_rendering.exe`。
- 示例开关：`Examples/CMakeLists.txt` 会自动生成 `BUILD_EXAMPLE_<NAME>` 选项，可在配置阶段关闭特定子目录。
- 新增示例：在 `Examples/your_demo/` 下创建源文件与 `CMakeLists.txt`，调用 `corona_add_example(NAME ... SOURCES ... COPY_ASSETS)`，再重新运行配置；该函数会自动链接 `CoronaEngine`、复制共享资产并安装运行时依赖。
- 运行时依赖（oneTBB、Python DLL）由 `corona_install_runtime_deps` 处理；为新的可执行目标调用即可拷贝 DLL/PDB。
- 编辑器前端/后端的依赖脚本位于 `Editor/CoronaEditor/build.py`，执行后会安装 Python 模块并使用打包的 Node/npm 完成 Web 构建。

## 🧷 代码习惯与注意事项
- 日志统一使用 `Utility/Logger` 提供的宏（`CE_LOG_INFO` 等）；`LogConfig` 在 `Engine::Init()` 时传入。
- 数学库使用 `ktm`（`Env/ktm`），矩阵/向量构造请依循 `ktm::translate3d` 等 API。
- 新系统若需不同节奏，可在构造函数调用 `SetTargetFps()`；记得在 `onStop()` 清理本地缓存并适时 `Engine::Instance().Shutdown()` 释放资源。
- 示例新增请使用 `Examples/corona_add_example()`，并考虑 `COPY_ASSETS` 复制共享资源。
- 目录 `Env/` 下封装三方二进制（Python 3.13.7、oneTBB、spdlog）；避免硬编码绝对路径，改用已有宏或 CMake 变量。

## 🧹 静态检查与命名约定
- 仓库根目录的 `.clang-tidy` 采用 `google-*` 检查族，关闭了 `google-build-using-namespace` 与 `google-readability-todo`，默认命名规则如下：
	- 类 / 结构体 / 接口 / 枚举使用 `CamelCase`。
	- 普通函数使用 `CamelCase`（与现有 API 保持一致），
	- 变量采用 `snake_case`，成员变量追加后缀 `_`。
	- 常量（含枚举值、成员常量、全局常量）使用 `kCamelCase`。
- 运行 clang-tidy 时建议限定在 `Src/`, `Utility/`, `Examples/`, `Editor/` 目录，减少对第三方头文件的噪音；可通过 `clang-tidy -p build path/to/file.cpp` 使用。
- 若新增命名风格不希望受到约束，可在本地调整 `.clang-tidy`，但提交前务必确保与仓库配置兼容。
