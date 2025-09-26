# CMake 构建与示例工程指引

本文介绍如何使用仓库提供的 CMake 预设完成快速构建，并示例说明如何添加新的 `Examples/` 示例程序。

## 快速构建流程

### 1. 配置构建目录

使用仓库内置的 Ninja Multi-Config 预设即可完成一次性配置：

```powershell
cmake --preset ninja-mc
```

常用 CMake 选项（均在 `Misc/cmake/CoronaOptions.cmake` 中定义）：

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `CORONA_BUILD_EXAMPLES` | `ON` | 是否编译 `Examples/` 下的演示程序 |
| `BUILD_CORONA_EDITOR` | `OFF` | 是否编译编辑器相关工程 |
| `BUILD_SHARED_LIBS` | `OFF` | 是否将核心库构建为共享库 |

你可以在配置阶段覆盖这些选项，例如：

```powershell
cmake --preset ninja-mc -DCORONA_BUILD_EXAMPLES=OFF -DBUILD_CORONA_EDITOR=ON
```

### 2. 构建目标

完成配置后即可按照需要选择构建配置，例如调试版：

```powershell
cmake --build --preset ninja-debug
```

其它可用预设包括 `ninja-release`、`ninja-rel_with_debinfo`、`ninja-min_size_rel` 等，与标准 CMake 多配置生成器一致。

## 管理示例工程

### 单个示例开关

`Examples/CMakeLists.txt` 会为每个子目录生成形如 `BUILD_EXAMPLE_RESOURCE_MANAGEMENT` 的布尔选项，默认开启。你可以在配置阶段逐项关闭：

```powershell
cmake --preset ninja-mc -DBUILD_EXAMPLE_RESOURCE_MANAGEMENT=OFF
```

启动项目（Visual Studio）会自动选择第一个开启的示例，你也可以在 IDE 中手动调整。

### 新增示例工程

1. 在 `Examples/` 下创建新的子目录（例如 `Examples/first_person_demo/`），并添加源文件：

   ```text
   Examples/
     first_person_demo/
       CMakeLists.txt
       first_person_demo.cpp
   ```

2. 在子目录 `CMakeLists.txt` 中使用仓库提供的 `corona_add_example` 辅助函数：

   ```cmake
   corona_add_example(
       NAME Corona_first_person_demo
       SOURCES first_person_demo.cpp
       COPY_ASSETS          # 如无需拷贝默认资源，可移除该行
   )
   ```

   - `NAME`：最终生成的可执行文件名。
   - `SOURCES`：示例的源文件列表，可一次性列出多个 `.cpp` / `.h`。
   - `COPY_ASSETS`（可选）：开启后会将 `Examples/assets/` 目录整体拷贝到目标输出目录，适合依赖通用资源的示例。

3. 重新执行配置命令，使新的示例被检测与构建：

   ```powershell
   cmake --preset ninja-mc
   ```

   若示例需要特定外部资源，可在 `Examples/assets/` 下添加共享内容，或在示例目录内自行管理复制逻辑。

> 提示：`corona_add_example` 函数已经自动链接 `glfw`、`CoronaEngine` 等依赖，并在可用时调用 `corona_install_runtime_deps` / `corona_install_corona_editor` 处理运行时 DLL 与编辑器资源，无需手工重复设置。

## 常见问题

- **Python 依赖检查失败**：配置阶段会自动调用 `Misc/pytools/check_pip_modules.py`，请确认系统 Python 或内置 `Env/Python-3.13.7` 可用，并按照提示安装缺失模块。
- **找不到示例子目录**：配置输出若出现 `[Examples] xxx subdir not found, skip`，请确认对应目录存在且包含 `CMakeLists.txt`。
- **资源复制过慢**：可在示例中去掉 `COPY_ASSETS` 并自行管理资产路径，或为示例单独编写更精细的复制命令。

完成上述步骤后，即可在 `build/bin/examples/`（或 IDE 生成目录）中找到编译好的示例可执行文件。
