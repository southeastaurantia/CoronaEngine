# CoronaEngine 开发者指南

## 快速上手
- 克隆仓库并初始化子模块（如果存在）。
- 安装符合 C++20 的现代编译工具链；各平台注意事项见《运行时依赖》(runtime-dependencies.zh.md)。
- 使用《CMake 指南》(cmake.zh.md) 中介绍的预设生成构建目录。
- 通过 `cmake --build` 构建引擎静态库与运行时可执行文件。

## 仓库结构
- `src/`：核心引擎代码，编译为 `CoronaEngine` 静态库。
- `engine/`：链接 `CoronaEngine` 的运行时可执行程序，负责主循环与生命周期管理。
- `examples/`：示例工程。为新特性或实验创建独立文件夹与示例。
- `misc/cmake/`：自定义 CMake 模块与辅助函数。
- `assets/`：运行时共享的资源文件，需要与二进制一起分发。

## 常见工作流
1. 在工程根目录执行 `cmake --preset ninja-msvc` 等配置命令。
2. 使用与配置预设匹配的构建预设，如 `cmake --build --preset msvc-debug`。
3. 新的引擎功能放在 `src/`，需要公开的接口通过头文件暴露。
4. 在 `examples/` 下新增示例验证功能或演示 API。
5. 遵循《代码风格》(code-style.zh.md)，必要时手动运行 clang-format 与 clang-tidy。

## 系统插件与服务注入
- 在调用 `RuntimeLoop::initialize()` 之前，可通过 `Engine::kernel().services()` 注册额外服务，为系统提供配置或诊断能力。
- 通过 `SystemRegistry::register_plugin` 扩展系统插件表，并在工厂函数中使用 `SystemContext` 获取注入的服务。
- `examples/minimal_runtime_loop` 示例现演示 `DiagnosticsSystem` 插件：注入 `DiagnosticsService`/`DiagnosticsConfig`，并利用 `CORONA_RUNTIME_SYSTEMS` 环境变量选择要启动的系统子集。
- 复用该模式可以为工具链或无渲染运行时按需选择系统，并统一管理依赖注入。

## 调试建议
- 利用 `CE_LOG_*` 宏开启详细日志，定位子系统问题。
- `Debug` 配置包含更多断言与日志；性能分析时可使用 `RelWithDebInfo`。
- 引擎模块化程度高，可按子系统逐步修改，并通过示例验证覆盖面。

## 提交清单
- 提交前格式化代码，确保符合风格规范。
- 引入新模块或流程时更新 `docs/` 下文档。
- 行为变化需提供示例或更新现有示例。
- 确认单元测试（若有）与示例在支持的预设上通过构建。
