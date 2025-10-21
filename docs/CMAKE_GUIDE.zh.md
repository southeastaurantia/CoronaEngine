# CoronaEngine CMake 指南

本文概述 CoronaEngine 仓库的 CMake 结构以及常见的扩展、构建流程。

## 快速上手
- 配置：在 Windows 使用 `cmake --preset ninja-msvc`，或在其他平台使用 `CMakePresets.json` 中合适的预设。
- 构建：执行 `cmake --build --preset msvc-debug`（或 `msvc-release` 等）。
- Ninja 多配置生成器会将所有配置生成在 `build/` 下；Visual Studio 预设同样在该目录生成解决方案。
- Debug 构建完成后可运行 `build/*/Debug/*.exe`。构建脚本会在后处理阶段自动复制资源、Python DLL 与其他运行时文件。

## 目录与文件概览
- `CMakeLists.txt`：负责总配置。扩展 `CMAKE_MODULE_PATH` 指向 `misc/cmake`，引入下述自定义模块，并按 `src -> engine -> examples` 顺序添加子目录。
- `CMakePresets.json`：定义跨平台的配置、构建预设。默认使用 Ninja Multi-Config，同时提供 Visual Studio 2022 及各平台专用预设。
- `src/CMakeLists.txt`：聚合模块库，创建静态库目标 `CoronaEngine`，并连接运行时依赖收集、可选的编辑器资源配置。
- `engine/CMakeLists.txt`：构建 `Corona_runtime` 可执行文件，安装 Helicon 运行时 DLL（若可用）、复制 Corona 运行时依赖、同步编辑器资源并拷贝 `assets/`。
- `examples/CMakeLists.txt`：声明辅助函数与子目录开关，通过 `corona_add_example()` 为示例配置统一的构建行为与资源同步逻辑。

## 自定义 CMake 模块（`misc/cmake`）
- `corona_options.cmake`：集中管理项目选项。包含 `BUILD_CORONA_RUNTIME`、`BUILD_CORONA_EDITOR`、`CORONA_BUILD_EXAMPLES`、`CORONA_BUILD_VISION` 等开关，默认在仓库顶层构建时启用运行时与示例。
- `corona_python.cmake`：固定使用 `third_party/Python-3.13.7` 内置 Python，配置 `misc/pytools/check_pip_modules.py` 进行依赖校验，并暴露 `check_python_deps` 自定义目标。若缺失包且禁用自动安装会直接失败。
- `corona_third_party.cmake`：枚举所有 FetchContent 依赖，包括自家模块（CabbageHardware、CabbageConcurrent、CoronaLogger、CoronaResource、Vision）与第三方库（assimp、stb、EnTT、glfw）。Vision、示例等特性会按开关拉取。
- `corona_compile_config.cmake`：统一 C11/C++20 标准、MSVC 运行库策略、EnTT 相关宏、Python 路径宏与 UTF-8 编译选项。
- `corona_collect_module.cmake`：提供 `corona_collect_module()` 用于在模块 `include/`、`src/` 中收集头文件与源文件，并导出 `CORONA_<MODULE>_PUBLIC_HEADERS` 等变量。同时定义 `corona_target_use_public_includes()` 以附加共享 `include/` 目录。
- `corona_runtime_deps.cmake`：提供 `corona_configure_runtime_deps()` 收集 Python DLL/PDB 并挂载到 `CoronaEngine`，以及 `corona_install_runtime_deps()` 复制依赖到可执行文件旁（优先使用 Python 脚本，备选 `copy_if_different`）。
- `corona_editor.cmake`：与运行时依赖模式相同，用于收集/复制 CabbageEditor 资源。启用 `BUILD_CORONA_EDITOR` 时，会将 Backend/Frontend 目录记录在 `CoronaEngine` 上，并在运行时或示例目标上添加复制与可选 Node 构建步骤。

## 核心目标
- `CoronaEngine`：汇聚所有引擎模块的静态库。链接 `src/*` 下声明的模块目标，并通过 `corona_target_use_public_includes()` 导出公共头文件。
- `Corona_runtime`：运行默认循环的主可执行文件，继承运行时依赖复制流程。
- `minimal_runtime_loop`：演示如何在外部复用 `engine/RuntimeLoop.cpp`，并展示示例如何同步资源与运行时依赖。

## 配置选项
| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `OFF` | 以共享库构建所有库目标。 |
| `BUILD_CORONA_RUNTIME` | `ON` | 添加 `engine/` 目录中的运行时可执行文件。 |
| `BUILD_CORONA_EDITOR` | `OFF` | 启用编辑器资源收集及复制步骤。 |
| `CORONA_BUILD_EXAMPLES` | 顶层构建时 `ON` | 添加所有示例子目录与对应开关。 |
| `CORONA_BUILD_VISION` | `OFF` | 拉取并链接 Corona Vision 模块。 |
| `CORONA_CHECK_PY_DEPS` | `ON` | 配置阶段验证 Python 依赖。 |
| `CORONA_AUTO_INSTALL_PY_DEPS` | `ON` | 允许自动安装缺失的 Python 包。 |
| `CORONA_PYTHON_USE_EMBEDDED_FALLBACK` | `ON` | 强制使用内置 Python。 |

可在运行 `cmake --preset <configure-preset>` 时通过 `-D<OPTION>=<value>` 覆写，或使用 `cmake -S . -B build` 手动指定。

## 新增引擎模块流程
1. 在 `src/<module>` 下创建目录（可包含 `include/`、`src/`）。
2. 新建 `CMakeLists.txt`，调用 `corona_collect_module(<Name> ${CMAKE_CURRENT_SOURCE_DIR})`。
3. 使用生成的 `CORONA_<NAME>_PUBLIC_HEADERS`、`CORONA_<NAME>_PRIVATE_SOURCES` 创建新的 `add_library()` 目标。
4. 链接所需依赖，若头文件位于共享 `include/` 树中，调用 `corona_target_use_public_includes(<target>)`。
5. 在 `src/CMakeLists.txt` 的 `_corona_modules` 列表中追加该目标，以便 `CoronaEngine` 链接它。

## 示例工程
- `examples/` 下的每个目录可通过 `BUILD_CORONA_EXAMPLE_<目录名>` 开关启停。根 CMake 会根据目录名自动声明这些选项。
- 示例应调用 `corona_add_example()`，以继承运行时依赖复制、编辑器资源镜像与可选资产复制等默认行为。额外源文件（例如 `engine/RuntimeLoop.cpp`）可通过 `target_sources()` 附加。

## 运行时与编辑器资源
- Python 运行时文件：配置阶段由 `corona_configure_runtime_deps()` 收集，并通过生成的列表或 `copy_if_different` 复制到目标。
- `assets/`：运行时和启用 `COPY_ASSETS` 的示例都会将整个目录复制到输出目录。
- 编辑器资源：启用编辑器后，`corona_install_corona_editor()` 会调用 `misc/pytools/editor_copy_and_build.py` 并在 Windows 上使用 `editor/CabbageEditor/Env` 中的 Node 运行时。

## 故障排查
- 若 Python 检查失败，可运行 `cmake --build --preset <preset> --target check_python_deps` 查看 `check_pip_modules.py` 输出。
- FetchContent 会在配置阶段拉取依赖；若需强制刷新，可清理 `build/_deps/`。
- 当前 `cmake_minimum_required` 声明为 4.0（与预设保持一致）。实际使用建议 CMake 3.28 及以上。
- 对于多配置 Ninja，请在某些版本的 CMake 中使用 `cmake --build --preset msvc-release --config Release` 指定配置。

## 约定总结
- 新增帮助函数时优先放在 `misc/cmake/`，方便复用。
- 复制运行时文件时使用现有 Python 脚本，避免重复写入。
- 新增构建开关时在 `corona_options.cmake` 中声明，并同步更新本文档。
