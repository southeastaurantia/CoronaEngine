# 公共头文件梳理

> 目的：配合 `architecture_refactor_todo.md` 阶段一，记录各模块待迁移的公开头文件位置与目标 `src/include` 路径。

| 模块 | 当前公开头目录 | 目标 include 路径 | 备注 |
| --- | --- | --- | --- |
| `core` | `include/corona/core/` | `include/corona/core/` | `Engine.h` 现提供 `EngineFacade` + `using Engine` 兼容层；`detail/EngineKernel.h` 仍暴露给模板调用，后续计划进一步收敛；`testing/MockServices.h` 作为对外测试辅助 |
| `systems/interface` | `include/corona/interfaces/` | `include/corona/interfaces/` | `ISystem.h`、`ThreadedSystem.h` 已迁移，新增 `Concurrency.h`、`ServiceLocator.h`、`SystemContext.h`、`Services.h` |
| `systems/animation` | `include/corona/systems/` | `include/corona/systems/` | `AnimationSystem.h` 现位于统一 include 树，依赖 `ThreadedSystem` 与外部 `AnimationState` |
| `systems/audio` | `include/corona/systems/` | `include/corona/systems/` | `AudioSystem.h` 现通过 `corona::system::audio` 导出 |
| `systems/display` | `include/corona/systems/` | `include/corona/systems/` | `DisplaySystem.h` 已迁移 |
| `systems/rendering` | `include/corona/systems/` | `include/corona/systems/` | `RenderingSystem.h` 与 `rendering/SceneEvents.h` 均迁移；后续评估事件是否下移到核心 |
| `thread` | `include/corona/threading/` | `include/corona/threading/` | `SafeCommandQueue`、`SafeDataCache`、`EventBus` 现已迁移至统一 include 树；后续如需接口包装再补充 |
| `script/python` | `include/corona/script/` | `include/corona/script/` | Python API/Bridge/Hotfix/EngineScripts 已迁移，等待 API 收敛与注释清理 |
| `utils` | `include/corona/utils/` | `include/corona/utils/` | `compiler_features.h` 现作为共享宏对外吐出 |

> 当前：公开头均已集中到 `include/corona/**`，后续聚焦在清理多余 detail 头与统一 CMake 导出策略。