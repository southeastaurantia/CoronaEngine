# 架构解耦改造 TODO 列表

参照 `architecture_refactor_proposal.md`，将整体改造划分为三个阶段，每个阶段内列出优先任务。勾选 (☐/☑) 用于记录进展。

文件移动使用 git mv命令

抛弃现有public/private方式，改为传统的src/include方式

## 阶段一：接口抽离与依赖清点
- ☑ 梳理现有 `core`、`systems`、`thread`、`script` 等模块的公开头文件，标注哪些需迁移至接口层。
- ☑ 新建 `src/interfaces/` 目标 (`corona::interfaces`)，导出 `ISystem`、事件结构体、组件定义等纯头信息。
- ☑ 将 `CoronaEngineAPI.h` 中对 EnTT 的 include 替换为 `<entt/entt.hpp>`，并通过 CMake 暴露公共 include。
- ☑ 统计每个系统对 `Engine::instance()` 的调用，记录所需服务（资源、事件、命令队列等）。
- ☑ 为 `SafeCommandQueue`、`SafeDataCache`、`EventBusT` 等并发工具补充最小接口包装，方便后续注入。

## 阶段二：内核/服务重构
- ☑ 创建新的 `EngineKernel` 与 `ServiceLocator` 实现，引入 `SystemContext` 结构。
- ☑ 抽取资源、日志、并发等核心服务到独立组件 (`IResourceService`、`ILogger`、`ICommandScheduler`)。
- ☑ 拆分 `Engine` 为对外兼容的 `EngineFacade`（旧 API）+ 内部 `EngineKernel`。
- ☑ 更新 CMake：`EngineKernel` 仅链接 `corona::interfaces` 与服务层，删除对系统静态库的循环依赖。
- ☑ 为测试构建简单 mock 服务，验证系统在无全局单例的情况下可启动。

## 阶段三：系统插件化与运行时装配
- ☑ 设计 `SystemPluginDescriptor`（名称、依赖、工厂），并实现插件注册表 `SystemRegistry`。
- ☑ 改造各系统构造函数，使用 `SystemContext` 注入服务，移除对 `Engine::instance()` 的直接调用。
- ☑ 将命令队列注册改为 `ICommandScheduler::create_queue(name, config)`，添加重复名检测。（当前实现维持原接口，已统一通过调度器创建并记录队列句柄）
- ☑ 在 `RuntimeLoop` 中通过插件表启停系统，支持按配置启用子集。
- ☐ 更新示例与文档，演示插件选择与服务注入流程。

## 阶段四：目录结构与构建脚本统一化
- ☑ 盘点 `src/core`, `src/systems`, `src/thread`, `src/utils`, `engine`, `script` 的公开头文件，规划迁移后的 `include/` 树结构（保留最小对外 API）。详见 `docs/phase4_migration_plan.md`。
- ☑ 将 `include/corona/**` 与 `src/core/public/CoronaEngineAPI.h` 迁移到仓库根部统一的 `include/` 目录，并调整命名空间/包含路径（已更新引用与 CMake include 路径）。
- ☐ 逐模块将 `public/private` 目录替换为 `src/` + 新 `include/` 结构，移动源文件并更新 `#include`。（核心、线程、系统、脚本、工具模块的公开头已迁移至 `include/corona/**`，后续整合私有实现与剩余目录结构）
- ☐ 清理 CMake：去除安装规则、统一 target/include 目录声明，确保在新目录结构下仍可成功配置与编译。
- ☐ 迁移过程中记录/删除冗余代码与重复接口，并在每次子步骤后更新此列表。

## 验收与后续
- ☐ 编写回归测试（至少覆盖系统启动/停止、资源加载、事件分发）。
- ☐ 完成文档更新：开发者指南、模块 README、迁移注意事项。
- ☐ 评估进一步拆分（如将脚本、渲染系统移至独立仓库）的可行性，并记录在后续计划中。
