# CMake 指南

## 预设概览
工程提供 `CMakePresets.json` 以统一配置与构建流程。

- `ninja-msvc`：Windows，使用 MSVC 与 Ninja Multi-Config。
- `ninja-clang`：Windows，使用 LLVM clang/clang++。
- `vs2022`：Windows，使用 Visual Studio 2022 生成器。
- `ninja-linux-gcc`、`ninja-linux-clang`：Linux，分别针对 GCC 与 Clang。
- `ninja-macos`：macOS，使用 Ninja Multi-Config。

所有预设继承自隐藏的 `base` 预设，默认设置：
- `binaryDir` = `${sourceDir}/build`
- `installDir` = `${sourceDir}/install`
- `CMAKE_EXPORT_COMPILE_COMMANDS` = `ON`（方便 IDE 与分析工具读取编译命令）

## 配置
在项目根目录选择适合的平台预设执行配置，例如 Windows + MSVC：

```powershell
cmake --preset ninja-msvc
```

或 Linux + Clang：

```bash
cmake --preset ninja-linux-clang
```

配置阶段会使用 `FetchContent` 拉取第三方依赖，并在 `build/` 下生成 Ninja 文件。

## 构建
使用与配置预设匹配的构建预设，例如：

```powershell
cmake --build --preset msvc-debug
```

可选构建类型包括 `Debug`、`Release`、`RelWithDebInfo` 与 `MinSizeRel`。

## 添加目标
- 核心引擎位于 `src/`，其 `CMakeLists.txt` 负责生成 `CoronaEngine` 静态库。
- 运行时（`engine/`）链接 `CoronaEngine` 并实现 `main` 函数。
- 示例集中在 `examples/`，每个子目录包含独立 `CMakeLists.txt`，需调用 `corona_add_example()`。
- 自定义构建逻辑存放于 `misc/cmake/`，优先复用或扩展现有助手函数。

## 小贴士
- 修改 CMake 脚本或依赖后请重新执行配置命令。
- 需要干净配置时，可删除 `build/` 或改用新的二进制目录。
- 通过 `cmake --install --preset <build-preset>` 将产物与资源复制到对应 `installDir`。
