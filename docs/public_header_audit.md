# 公共头文件梳理

> 目的：配合 `architecture_refactor_todo.md` 阶段一，记录各模块待迁移的公开头文件位置与目标 `src/include` 路径。

| 模块 | 当前公开头目录 | 目标 include 路径 | 备注 |
| --- | --- | --- | --- |
| `core` | `include/corona/core/` | `include/corona/core/` | `Engine.h` 现提供 `EngineFacade` + `using Engine` 兼容层；`detail/EngineKernel.h` 仍暴露给模板调用，后续计划进一步收敛；`testing/MockServices.h` 作为对外测试辅助 |
| `systems/interface` | `src/include/corona/interfaces/` | `src/include/corona/interfaces/` | `ISystem.h`、`ThreadedSystem.h` 已迁移，新增 `Concurrency.h`、`ServiceLocator.h`、`SystemContext.h`、`Services.h` |
| `systems/animation` | `src/systems/animation/public/` | `src/include/corona/systems/animation/` | 依赖 `ThreadedSystem` 和 `AnimationState`，待合并至接口层 |
| `systems/audio` | `src/systems/audio/public/` | `src/include/corona/systems/audio/` | 头文件引用已指向接口库 |
| `systems/display` | `src/systems/display/public/` | `src/include/corona/systems/display/` | 同上 |
| `systems/rendering` | `src/systems/rendering/public/` | `src/include/corona/systems/rendering/` | `RenderingSystem.h` 已使用新接口路径，事件头待迁移 |
| `thread` | `include/corona/threading/` | `include/corona/threading/` | `SafeCommandQueue`、`SafeDataCache`、`EventBus` 现已迁移至统一 include 树；后续如需接口包装再补充 |
| `script/python` | `src/script/python/public/` | `src/include/corona/script/python/` | Python API 头文件保持对外暴露，后续统一到 include 下 |
| `utils` | `src/utils/public/` | `src/include/corona/utils/` | `compiler_features.h` 已满足 include 结构，待移动 |

> 下一步：逐步将各模块 `public/` 目录中的头文件迁往 `src/include/corona/...`，并在 CMake 中统一通过 `corona::interfaces` 或对应子库导出。