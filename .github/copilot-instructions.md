# CoronaEngine Copilot 指南

## 项目架构概览
- **核心引擎**：`src/core`（包含 `engine`、`systems`、`thread` 等）与 `src/resource`（资源管理）
- **公共 API**：`src/include`（对外暴露的公共头文件，如 `CoronaEngineAPI.h`）
- **脚本支持**：`src/script/python` 提供 Python 内嵌脚本入口
- **通用组件**：
  - `src/utility/logger`：统一日志系统
  - `src/utility/resource_manager`：资源管理器
  - `src/utility/concurrent`：并发工具集
  - `src/common/include`：跨模块共享的通用头文件
- **第三方依赖**：由 `misc/cmake/corona_third_party.cmake` 通过 FetchContent 统一管理（assimp、EnTT、GLFW、Vulkan 等）
- **示例程序**：`examples/` 目录，其中 `interactive_rendering` 展示了完整的系统注册、缓存操作和渲染输出流程，是学习交互逻辑的最佳参考

## 引擎生命周期与线程模型

### 核心生命周期
- **启动流程**：`Engine::Instance().Init(cfg)` → `RegisterSystem<T>()` → `StartSystems()`
- **关闭流程**：`Shutdown()` 依次停止所有系统并清理资源

### 线程模型
- **系统继承**：所有运行时系统继承 `ThreadedSystem`（默认 120 FPS）
- **执行模型**：`onTick()` 在专用线程中调用，可通过 `SetTargetFps` 自定义帧率
- **线程安全交互**：必须通过 `SafeCommandQueue` 与系统交互
  - 注册时：系统构造函数调用 `Engine::AddQueue(name(), std::make_unique<SafeCommandQueue>())`
  - 使用时：业务线程用 `Engine::GetQueue(name()).enqueue(...)` 投递任务

### 高层 API
- `CoronaEngineAPI`（`src/include/CoronaEngineAPI.h`）封装 actor/scene 生命周期管理,作为公共 API 头文件暴露
- 实现文件：`src/core/CoronaEngineAPI.cpp`
- 内部机制：使用 `DataId::Next()` 生成唯一 ID、缓存写入和系统队列
- 目标用户：外部模块（Python 脚本、客户端应用）
- 使用方式：`#include <CoronaEngineAPI.h>`

## 数据缓存与资源加载

### 数据缓存系统
- **获取缓存**：使用 `Engine::Cache<T>()` 获取 `SafeDataCache<T>` 实例
- **ID 管理**：插入前调用 `DataId::Next()` 分配唯一 ID
- **数据修改**：通过 callback 方式修改（参考 `SafeDataCache::modify`）
- **安全遍历**：使用 `safe_loop_foreach` 遍历场景/模型，该函数处理 try-lock 重试
  - ⚠️ **重要**：回调函数应保持简短，避免阻塞系统线程

### 资源管理系统
- **基础功能**：`ResourceManager`（`src/utility/resource_manager`）默认启用资源缓存
- **加载方式**：
  - `load()`：返回共享指针，适合常规加载
  - `loadOnce()`：一次性读取，不缓存
  - `loadAsync()`：后台异步加载
- **扩展开发**：自定义 loader 实现参考 `examples/resource_management/resource_management.cpp`

## utility 模块设计约定

### logger 模块（`src/utility/logger`）
- **接口封装**：仅通过 `<Log.h>` 对外暴露，内部隐藏 spdlog 实现细节
- **初始化**：在入口点（如 `Engine::Init`）调用 `Logger::Init`
- **使用规范**：业务代码统一使用 `CE_LOG_*` 宏，禁止直接调用第三方日志 API

### resource_manager 模块（`src/utility/resource_manager`）
- **Loader 注册**：所有 Loader 必须在引擎初始化阶段注册
- **路径标准化**：`ResourceId::ComputeUid` 统一处理路径格式（小写 + 正斜杠 `/`）
- **类型扩展**：新增资源类型时复用 UID 生成逻辑，架构应放在 `ResourceTypes` 下

### concurrent 模块（`src/utility/concurrent`）
- **目录分层**：`core/` 提供原子、线程、内存与同步原语，`container/` 聚合各类并发容器，`util/` 包含回收器、线程池与基准工具；公共入口统一通过 `include/concurrent.h`
- **缓存一致性**：复用 `CacheLineAligned`、`CacheLinePadding` 等工具避免伪共享；对跨线程共享的计数器或指针保持 64B 对齐，必要时补齐填充字段
- **容器扩展**：新增容器默认采用 shard/stripe 方案，保持接口与 STL 语义一致；在 Traits 中显式声明哈希、比较与重哈希策略，并在 `bench/` 下补充微基准
- **内存回收**：优先使用 `EpochReclaimer` 获取高吞吐；若需要强一致指针语义，改用 `HazardPointer` 或混合策略，并确保在单元测试中覆盖并发释放路径
- **线程与任务工具**：`ThreadPool` 采用工作窃取调度，提交任务使用 `submit(Priority, Fn, Args...)`；在引擎系统中结合 `SpinWait`、`cpu_relax` 等退避原语控制自旋成本
- **设计文档**：复杂改动需同步更新 `utility/concurrent/detail.md` 与 README 中的性能数据/指标，保持与实测结果一致

### 通用模块开发规范
- **目录结构**：源码放在模块根目录，公共头文件集中在 `include/` 子目录
- **CMake 集成**：通过根目录 `CMakeLists.txt` 暴露公共接口
- **设计原则**：保持线程安全，提供跨系统通用能力
- **使用范围**：供 `src/core` 和 `examples` 共享使用
- **公共接口约定**：导出头文件需通过 `src/utility/<module>/include` 聚合，示例代码使用统一的 umbrella 头（如 `include/concurrent.h`），避免直接依赖内部 `core/`、`detail/` 文件
- **文档同步**：模块改动后同步更新 `README.md`、`detail.md` 以及相关报告（例如 `utility_common_consolidation_report.md`、`corona_common_implementation_report.md`），保持设计说明与性能数据一致
- **测试与基准**：功能更新至少覆盖一条 Catch2 单元测试或 `examples/` 中的演示；性能敏感改动须更新 `bench/` 目录基准或在 `run_performance_tests*.ps1` 中记录新数据
- **共享依赖**：跨模块复用的工具放入 `src/common/include` 或各模块 `core/` 层，通过轻量适配器暴露，禁止形成循环依赖
- **CMake 目标**：新增 utility 模块需在 `src/utility/CMakeLists.txt` 注册独立 target，并确保顶层构建透传公共 include path、编译选项与必要的第三方依赖

## 渲染与动画协作
- `RenderingSystem`（`src/core/engine/systems/RenderingSystem.*`）在 `onTick()` 消费命令队列后调用 `updateEngine()`：遍历 `Scene` 与 `Model` 缓存、刷新 g-buffer、写回 `HardwareImage`，并将最终输出绑定到 `Scene::displayer`。
- `AnimationSystem` 通过 `state_cache_keys_` 与 `model_cache_keys_` 管理骨骼动画和碰撞；`updateAnimationState` 会填充 `AnimationState::bones` 并刷新 `Model::bonesMatrixBuffer`。
- 向渲染或动画系统注册资源时务必调用 `WatchScene` / `WatchModel`（见 `CoronaEngineAPI::Scene`、`CoronaEngineAPI::Actor` 构造函数），销毁前对称调用 `Unwatch*` 以避免悬挂引用。
- 需要新增系统时，沿用 `ThreadedSystem` + `SafeCommandQueue` 模式，并在 `Engine::StartSystems()` 前完成 `RegisterSystem`。

## Python 与编辑器集成
- `misc/cmake/corona_python.cmake` 会优先检测系统 Python≥`CORONA_PYTHON_MIN_VERSION`，否则回退到 `third_party/Python-3.13.7`；配置阶段默认执行 `misc/pytools/check_pip_modules.py` 校验 requirements。
- `src/script/python/PythonAPI.*` 将 `Editor/CoronaEditor/Backend` 打包为嵌入式模块 `CoronaEngine`，内置热更新（`PythonHotfix`）与 `PyInit_CoronaEngineEmbedded` 类型注册。
- 构建编辑器资源需开启 `-DBUILD_CORONA_EDITOR=ON`，随后 `corona_install_corona_editor` 调用 `misc/pytools/editor_copy_and_build.py` 使用 `third_party/node-v22.19.0` 运行 `npm install && npm run build`；错误只发出警告但不会终止构建。

## 构建与运行工作流
- 首次配置：`cmake --preset ninja-mc`（PowerShell）；常用构建 `cmake --build --preset ninja-debug --target Corona_interactive_rendering`，其他配置参见 `CMakePresets.json`。
- 关键选项：`CORONA_BUILD_EXAMPLES` 控制示例，`BUILD_CORONA_EDITOR` 控制编辑器资源，`BUILD_SHARED_LIBS` 默认为 OFF；所有开关定义在 `misc/cmake/corona_options.cmake`。
- 运行示例前确保 `misc/pytools/check_pip_modules.py` 通过（如需手动复查可执行 `cmake --build --preset ninja-debug --target check_python_deps`）。
- 生成的可执行与依赖 DLL 会被 `corona_install_runtime_deps` 复制到目标目录；如添加新依赖，请更新 `misc/cmake/corona_runtime_deps.cmake`。

## examples 开发规范

### 添加新示例
- **注册方式**：在 `examples/CMakeLists.txt` 中使用 `corona_add_example` 函数
- **必要参数**：`NAME`（示例名称）、`SOURCES`（源文件列表）
- **可选参数**：`COPY_ASSETS`（控制是否拷贝 `examples/assets/` 目录）

### 构建配置
- **构建开关**：每个示例自动生成 `BUILD_EXAMPLE_<NAME>` 开关，默认启用
- **依赖链接**：示例目标自动链接 `CoronaEngine` 库和 `glfw`
- **选择性构建**：可在 CMake 配置时逐项关闭不需要的示例

### 运行环境
- **工作目录**：示例运行目录位于 `$<TARGET_FILE_DIR>`（可执行文件所在目录）
- **调试配置**：`VS_DEBUGGER_WORKING_DIRECTORY` 已自动设置
- **资产访问**：使用 `std::filesystem::current_path()/assets` 访问资源文件

## 代码规范与调试指南

### 日志规范
- **统一接口**：使用 `CE_LOG_*` 宏（定义在 `src/utility/logger/include/Log.h`）
- **级别配置**：默认日志级别在 `Engine::Init` 时设置，可通过 `Logger::SetLevel` 自定义

### 资源管理规范
- **ID 类型**：资源 ID 和缓存 key 统一使用 `uint64_t` 类型
- **ID 分配**：禁止手工复用旧 ID，必须通过 `DataId::Next()` 分配
- **生命周期**：销毁对象时必须调用 `erase` 清理缓存，并通知相关系统取消关注

### 渲染资源管理
- **硬件抽象**：渲染资源（`HardwareImage`、`HardwareBuffer` 等）由 `CabbageHardware` 提供
- **线程同步**：在 CPU 线程创建资源后，通过命令队列与 GPU 线程同步

### 路径处理规范
- **基础路径**：资产路径基于运行目录的 `assets/` 子目录
- **跨平台兼容**：保持正斜杠 `/` 格式，复用 `ResourceId::ComputeUid` 的标准化逻辑

### 命名约定（遵循 `.clang-tidy` google-readability 配置）
- **类型命名**：类/结构体/接口/枚举使用 `CamelCase`
- **函数变量**：自由函数和局部变量使用 `snake_case`
- **成员变量**：使用 `snake_case_` 格式（自动追加下划线 `_`）
- **常量枚举**：使用 `kCamelCase` 前缀
- **文件命名**：使用 `CamelCase` 格式
- **命名空间**：使用 `CamelCase` 格式
- **特殊情况**：main 风格入口函数可适当放宽检查

## 性能基准与验证流程
- **并发容器示例**：`cmake --build --preset ninja-debug --target Corona_concurrent_containers` 后运行对应可执行，验证 `ConcurrentHashMap`、`MPMCQueue` 等核心路径；提交前记录 ops/s、延迟与线程数配置
- **性能脚本**：仓库提供 `run_performance_tests.ps1`、`run_performance_tests_simple.ps1` 与英文说明版脚本，默认从项目根目录执行，输出数据同步至 `performance_test_results.txt`
- **数据追踪**：若调优缓存或哈希策略，需更新 `cache_line_optimization_report.md`、`sharding_optimization_results.md` 等报告，保持基线对比可追溯
- **监控指标**：基准测试至少包含吞吐、p99 延迟与内存占用；异常结果需附上硬件配置、编译选项（如 `-O3`、`-march=native`）与对照组说明

## 版本控制排除目录
以下目录已配置在 `.gitignore` 中，不应提交到版本控制：
- `build/` - CMake 构建输出目录
- `.cache/` - 各种缓存文件
- 其他临时文件和依赖下载目录（参见项目根目录 `.gitignore` 文件）
