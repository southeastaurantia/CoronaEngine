# CoronaEngine 架构解耦提案

> 目标：在保持现有功能的前提下，降低核心模块与各系统之间的耦合度，优化构建依赖链路，并为后续扩展（系统新增、平台移植、动态装配）打下基础。

## 现状问题小结
- **核心-系统环状依赖**：`CoronaCore` 链接 `corona::system::*`，系统实现又直接引用核心头文件与 `Engine::instance()`，静态库之间的相互依赖导致链接顺序敏感、编译时间长。
- **全局单例侵入**：系统通过全局单例访问资源管理器、事件总线与命令队列，拓展/测试困难。
- **接口与实现混杂**：`public` 目录同时包含实现所需的重量头文件，外部调用者被迫包含大量间接依赖。
- **第三方路径耦合**：诸如 `CoronaEngineAPI.h` 直接引用 `build/_deps` 下的 EnTT 头文件，降低可移植性。
- **装配逻辑硬编码**：系统注册由 `Engine`/`RuntimeLoop` 手写，缺乏插件化与条件构建能力。

## 设计原则
1. **稳定分层**：核心内核（生命周期、服务容器）与领域系统（渲染/动画/音频）彻底分离，禁止跨层反向依赖。
2. **显式依赖注入**：通过接口和服务定位器（或构造注入）向系统提供资源，而不是直接访问全局单例。
3. **接口优先**：将公共接口与数据契约迁移到 `include/` 或 `src/interfaces/`，实现细节留在 `private`。
4. **模块即插件**：系统以“插件描述符”注册，允许根据构建选项或运行时配置裁剪。
5. **构建独立性**：所有模块仅依赖目标导出的 include 目录，禁止引用 `build/_deps` 路径。

## 新的模块划分
```
┌──────────────────────────────────────────┐
│             CoronaEngine SDK            │
├───────────────┬───────────────┬─────────┤
│ Interfaces    │ Services      │ Tooling │
│ (pure header) │ (core impl)   │ (Python │
│               │               │ bridge) │
└───────────────┴───────────────┴─────────┘
        ▲                    ▲
        │                    │
┌───────┴─────────┐   ┌──────┴──────────┐
│ Runtime Systems │   │ Runtime Shell   │
│ (render/audio/  │   │ (engine app /   │
│ animation/...)  │   │ examples)       │
└─────────────────┘   └─────────────────┘
```

### 1. Interfaces 层（新建 `src/interfaces/`）
- 发布 `ISystem`、`IService`、`IResourceLoader` 等纯虚接口。
- 包含 `Systems.h`（定义系统配置结构）、`Events.h`、`Components.h`。
- 仅依赖 STL、数学库、EnTT 公共头，不出现实现细节。

### 2. Services 层（原 `core`、`thread`、`utils` 重组）
- **核心内核 (`EngineKernel`)**：负责系统生命周期管理、服务注册、事件中心。提供 `EngineKernel::install_plugin(const IPlugin&)` 接口。
- **并发工具 (`corona::concurrency`)**：保留 `SafeCommandQueue`、`EventBus`、`SafeDataCache`，但通过纯接口 `ICommandQueue` 暴露。
- **资源管理 (`corona::resource`)**：对 `CoronaResource` 封装，暴露 `IResourceService` 接口。
- **日志与诊断 (`corona::diagnostics`)**：统一封装 `CoronaLogger`，提供 `ILogger`/`ILogSink`。

### 3. Runtime Systems（渲染/动画/音频/显示等）
- 每个系统实现 `ISystem`，并通过 `SystemPluginDescriptor` 描述自身需求：
  ```cpp
  struct SystemPluginDescriptor {
      std::string_view name;
      std::vector<ServiceRequirement> requires;
      std::function<std::unique_ptr<ISystem>(SystemContext&)> factory;
  };
  ```
- `EngineKernel` 在启动时读取插件清单，解析依赖关系并顺序启动。

### 4. Tooling/Python 层
- Python API、热更新与桥接作为独立服务 `PythonScriptingService`，实现 `IScriptingService` 接口；由系统通过服务容器请求使用。

### 5. Runtime Shell（`engine/`、`examples/`）
- 仅负责：读取配置 -> 构建 `EngineKernel` -> 装载选定插件 -> 驱动主循环。
- `RuntimeLoop` 改为依赖 `EngineKernel::for_each_system(...)`，而非直接 `engine.get_system<T>()`。

## 关键解耦策略
### 1. 插件式系统注册
- 新建 `plugins/` 目录，每个系统提供 `CoronaRegister<RenderingSystemPlugin>(PluginRegistry&)` 函数。
- `EngineKernel` 装配阶段：
  1. 解析插件依赖图。
  2. 为每个系统创建专属命令队列（由 `concurrency` 服务自动命名与管理）。
  3. 初始化系统时传入 `SystemContext`（包含事件总线、资源服务、调度器）。

### 2. 服务定位器与依赖注入
- `SystemContext`（由 `EngineKernel` 创建）包含：
  ```cpp
  struct SystemContext {
      IServiceLocator& services;
      ICommandQueue&   queue;
      IEventHub&       events;
      IDataCacheHub&   caches;
  };
  ```
- 系统通过 `services.get<IResourceService>()` 获取资源能力，以接口形式耦合。

### 3. 公共接口包
- 移除系统对核心实现头文件的引用，例如：
  - `RenderingSystem` 改为包含 `RenderingSystemInterfaces.h`（内部只含所需接口、事件契约）。
  - `AnimationSystem` 通过 `IAnimationDataProvider` 访问模型骨骼信息，这个接口由资源服务实现。

### 4. 构建改进
- `src/interfaces` 作为 `INTERFACE` 目标 `corona::interfaces`，所有模块 `PUBLIC` 链接该目标。
- `EngineKernel`（替代现 `CoronaCore`）仅链接 `corona::interfaces` 与 `corona::services`；系统库仅 `PRIVATE` 链接 `corona::services`。
- 移除 `CoronaEngineAPI.h` 对 `build/_deps` 的依赖，改为 `#include <entt/entt.hpp>`，并在 CMake 中 `target_link_libraries(corona::interfaces PUBLIC EnTT::EnTT)`。

## 过渡实施步骤
1. **接口抽离**：
   - 新建 `src/interfaces/`，迁移 `ISystem`、`EventBusT` 模板声明、`ActorEvents`、`SceneEvents` 等数据结构。
   - 为现有系统添加适配层，短期内仍可访问旧的 `Engine::instance()`，但同时引入 `SystemContext`。
2. **内核重构**：
   - 将 `Engine` 拆分为 `EngineKernel`（纯 C++ 类）与 `EngineFacade`（保留对旧 API 的向后兼容包装）。
   - 引入 `ServiceLocator` 简易实现，并用以管理资源/日志/并发服务。
3. **系统插件化**：
   - 逐个系统改造构造函数，改为接受 `SystemContext`。
   - 引入插件注册表 `SystemRegistry`，由 `RuntimeLoop` 或 `EngineFacade` 注入。
4. **命令队列管理**：
   - 替换 `Engine::add_queue/get_queue` 为 `ICommandScheduler` 服务，负责命名、冲突检测与调度策略。
5. **清理旧依赖**：
   - 完成系统改造后，删除对 `Engine::instance()` 的直接调用。
   - 清理 `core` 与系统之间的 `target_link_libraries` 循环关系。
6. **文档与示例更新**：
   - 更新 `README`、`docs/developer-guide`，解释新装配流程。
   - 提供示例脚本展示如何在运行时选择性启用系统插件。

## 预期收益
- **编译/链接更稳定**：循环依赖被消除，目标构建顺序更易维护。
- **模块独立测试**：系统依赖接口，可通过 mock 服务进行单元测试。
- **可插拔扩展**：新增系统只需实现接口并注册插件，无需更改核心代码。
- **跨平台友好**：所有 include 都来自接口目标，减少路径硬编码。
- **运行时弹性**：可根据配置启停系统，支持多进程/多实例场景。

## 后续深化方向
- 结合依赖注入框架（如轻量版 Service Locator 或模板元编程 DI）。
- 将事件总线与数据缓存抽象为网络/分布式友好的协议，方便远程监控或云端渲染。
- 在插件描述符中增加能力声明（线程模型、资源需求），为自动调度奠定基础。
- 打包接口层为独立 SDK，供第三方插件开发。
