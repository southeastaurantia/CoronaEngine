# CMake Build Guide

CoronaEngine CMake 构建系统完整指南

---

## 目录

- [快速开始](#快速开始)
- [CMake 版本要求](#cmake-版本要求)
- [预设配置](#预设配置)
- [构建选项](#构建选项)
- [模块化 CMake 脚本](#模块化-cmake-脚本)
- [添加新组件](#添加新组件)
- [CMake 函数参考](#cmake-函数参考)
- [依赖管理](#依赖管理)
- [常见问题](#常见问题)
- [高级用法](#高级用法)

---

## 快速开始

### Windows (PowerShell)

```powershell
# 1. 配置项目（使用 Clang）
cmake --preset ninja-clang

# 2. 构建（Debug 配置）
cmake --build build --config Debug

# 3. 构建（Release 配置）
cmake --build build --config Release

# 4. 运行示例
.\build\examples\interactive_rendering\Debug\Corona_interactive_rendering.exe
```

### Linux / macOS

```bash
# 1. 配置项目
cmake --preset ninja-linux-clang  # Linux
cmake --preset ninja-macos         # macOS

# 2. 构建
cmake --build build --config Debug

# 3. 运行示例
./build/examples/interactive_rendering/Debug/Corona_interactive_rendering
```

---

## CMake 版本要求

**最低版本**: CMake 4.0.0

```cmake
cmake_minimum_required(VERSION 4.0)
```

**关键特性依赖**:
- `FetchContent` 用于第三方依赖管理
- Multi-Config generators (Ninja Multi-Config)
- Generator expressions 用于配置相关逻辑
- Target properties 用于模块化配置

---

## 预设配置

项目使用 `CMakePresets.json` 提供多种编译器和平台的预设配置。

### 可用预设

#### Windows

| 预设名 | 生成器 | 编译器 | 描述 |
|--------|--------|--------|------|
| `ninja-clang` | Ninja Multi-Config | Clang (GNU style) | **默认推荐** |
| `ninja-msvc` | Ninja Multi-Config | MSVC | Microsoft 编译器 |
| `vs2022` | Visual Studio 17 2022 | MSVC | VS IDE 集成 |

#### Linux

| 预设名 | 生成器 | 编译器 | 描述 |
|--------|--------|--------|------|
| `ninja-linux-clang` | Ninja Multi-Config | Clang | LLVM Clang |
| `ninja-linux-gcc` | Ninja Multi-Config | GCC | GNU 编译器 |

#### macOS

| 预设名 | 生成器 | 编译器 | 描述 |
|--------|--------|--------|------|
| `ninja-macos` | Ninja Multi-Config | 系统默认 | AppleClang |

### 预设特性

所有预设共享以下配置：
- **二进制目录**: `${sourceDir}/build`
- **安装目录**: `${sourceDir}/install`
- **编译数据库**: 自动生成 `compile_commands.json`

---

## 构建选项

### 核心选项

| 选项 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| `BUILD_SHARED_LIBS` | BOOL | `OFF` | 构建共享库（DLL/SO）而非静态库 |
| `CORONA_BUILD_EXAMPLES` | BOOL | `ON` | 构建示例程序 |
| `BUILD_CORONA_EDITOR` | BOOL | `OFF` | 构建 Corona 编辑器 |

### Python 相关选项

| 选项 | 类型 | 默认值 | 描述 |
|------|------|--------|------|
| `CORONA_PYTHON_MIN_VERSION` | STRING | `3.13` | 最低 Python 版本 |
| `CORONA_PYTHON_USE_EMBEDDED_FALLBACK` | BOOL | `ON` | 未找到系统 Python 时使用嵌入式版本 |
| `CORONA_CHECK_PY_DEPS` | BOOL | `ON` | 配置时检查 Python 依赖 |
| `CORONA_AUTO_INSTALL_PY_DEPS` | BOOL | `ON` | 自动安装缺失的 Python 包 |

### Utility 模块选项

自动为每个 utility 模块生成 `BUILD_CORONA_<MODULE>` 选项：

| 选项 | 默认值 | 描述 |
|------|--------|------|
| `BUILD_CORONA_LOGGER` | `ON` | 日志系统 |
| `BUILD_CORONA_RESOURCE_MANAGER` | `ON` | 资源管理器 |
| `BUILD_CORONA_CONCURRENT` | `ON` | 并发工具 |

### 示例选项

自动为每个示例生成 `BUILD_EXAMPLE_<NAME>` 选项：

| 选项 | 默认值 |
|------|--------|
| `BUILD_EXAMPLE_INTERACTIVE_RENDERING` | `ON` |
| `BUILD_EXAMPLE_PYTHON_SCRIPTING` | `ON` |
| `BUILD_EXAMPLE_RESOURCE_MANAGEMENT` | `ON` |
| `BUILD_EXAMPLE_MULTI_WINDOW_RENDERING` | `ON` |
| `BUILD_EXAMPLE_CONCURRENT_PERFORMANCE` | `ON` |

### 使用选项

```powershell
# 禁用示例构建
cmake --preset ninja-clang -DCORONA_BUILD_EXAMPLES=OFF

# 启用编辑器构建
cmake --preset ninja-clang -DBUILD_CORONA_EDITOR=ON

# 禁用特定 utility
cmake --preset ninja-clang -DBUILD_CORONA_LOGGER=OFF

# 禁用自动安装 Python 依赖
cmake --preset ninja-clang -DCORONA_AUTO_INSTALL_PY_DEPS=OFF
```

---

## 模块化 CMake 脚本

项目使用模块化的 CMake 脚本组织构建逻辑，所有模块位于 `misc/cmake/` 目录。

### 脚本概览

```
misc/cmake/
├── corona_options.cmake           # 全局构建选项
├── corona_python.cmake            # Python 探测与依赖管理
├── corona_compile_config.cmake    # 编译器配置与宏定义
├── corona_third_party.cmake       # 第三方依赖（FetchContent）
├── corona_runtime_deps.cmake      # 运行时依赖复制（DLL/PDB）
└── build_cabbage_editor.cmake     # 编辑器资源安装
```

### 1. corona_options.cmake

**职责**: 定义项目级全局构建选项

**导出选项**:
- `BUILD_SHARED_LIBS`
- `BUILD_CORONA_EDITOR`
- `CORONA_BUILD_EXAMPLES`

**使用**:
```cmake
include(corona_options)
```

---

### 2. corona_python.cmake

**职责**: Python 解释器发现、版本检查、依赖校验

#### 关键功能

1. **Python 探测**:
   - 优先查找系统 Python (≥ 3.13)
   - 回退到嵌入式 Python (`third_party/Python-3.13.7/`)

2. **依赖检查**:
   - 读取 `misc/pytools/requirements.txt`
   - 调用 `misc/pytools/check_pip_modules.py` 校验
   - 可选自动安装缺失包

3. **自定义目标**:
   - `check_python_deps`: 手动触发依赖检查

#### 导出变量

| 变量 | 描述 |
|------|------|
| `Python3_EXECUTABLE` | Python 解释器路径 |
| `Python3_VERSION` | Python 版本 |
| `Python3_INCLUDE_DIRS` | Python 头文件目录 |
| `Python3_LIBRARY_DIRS` | Python 库目录 |
| `Python3_RUNTIME_LIBRARY_DIRS` | Python 运行时目录 |

#### 工具函数

##### `corona_run_python(OUT_RESULT)`

统一的 Python 脚本执行接口。

**参数**:
- `SCRIPT`: Python 脚本路径（必需）
- `ARGS`: 脚本参数（可选）
- `WORKING_DIRECTORY`: 工作目录（可选，默认 `CMAKE_SOURCE_DIR`）

**输出变量**:
- `${OUT_RESULT}`: 退出码
- `CORONA_LAST_PY_STDOUT`: 标准输出
- `CORONA_LAST_PY_STDERR`: 标准错误

**示例**:
```cmake
corona_run_python(_result
    SCRIPT "${PROJECT_SOURCE_DIR}/scripts/my_script.py"
    ARGS --arg1 value1 --arg2 value2
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Script failed: ${CORONA_LAST_PY_STDERR}")
endif()
```

---

### 3. corona_compile_config.cmake

**职责**: 编译器配置、C/C++ 标准、编译宏定义

#### C/C++ 标准

```cmake
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
```

#### 全局编译宏

| 宏 | 平台 | 描述 |
|-----|------|------|
| `NOMINMAX` | All | 防止 Windows.h min/max 宏污染 |
| `_CRT_SECURE_NO_WARNINGS` | MSVC | 关闭 CRT 安全警告 |
| `CORONA_PY_EXECUTABLE` | All | Python 解释器路径（反斜杠转义） |
| `CORONA_PY_HOME` | All | Python 运行时主目录 |
| `CORONA_PY_DLLS` | All | Python DLLs 目录 |
| `CORONA_PY_LIB` | All | Python Lib 目录 |

#### 工具函数

##### `corona_to_backslash(INPUT OUT_VAR [ESCAPE_FOR_CSTRING])`

将路径中的正斜杠转为反斜杠（Windows 风格）。

**参数**:
- `INPUT`: 输入路径
- `OUT_VAR`: 输出变量名
- `ESCAPE_FOR_CSTRING`: 可选标志，双反斜杠转义用于 C 字符串

**示例**:
```cmake
corona_to_backslash("C:/Program Files/Python" _path)
# _path = "C:\Program Files\Python"

corona_to_backslash("C:/Program Files/Python" _path ESCAPE_FOR_CSTRING)
# _path = "C:\\Program Files\\Python"
```

---

### 4. corona_third_party.cmake

**职责**: 使用 FetchContent 管理第三方源码依赖

#### 已集成依赖

| 依赖 | 仓库 | 用途 |
|------|------|------|
| `assimp` | github.com/assimp/assimp | 3D 模型加载 (OBJ/FBX/GLTF) |
| `stb` | github.com/nothings/stb | 图像加载（stb_image.h） |
| `entt` | github.com/skypjack/entt | ECS (Entity Component System) |
| `CabbageHardware` | github.com/CoronaEngine/CabbageHardware | 硬件/平台抽象 |
| `glfw` | github.com/glfw/glfw | 窗口管理（仅示例） |

#### FetchContent 配置

所有依赖使用：
- `GIT_SHALLOW TRUE`: 浅克隆加速
- `EXCLUDE_FROM_ALL`: 不参与 ALL 构建目标

#### 添加新依赖

```cmake
FetchContent_Declare(
    my_library
    GIT_REPOSITORY https://github.com/user/my_library.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(my_library)
```

---

### 5. corona_runtime_deps.cmake

**职责**: 收集并复制运行时依赖（DLL/PDB）到可执行文件目录

#### 工作流程

1. **配置阶段** (`corona_configure_runtime_deps`):
   - 收集 TBB DLLs/PDBs
   - 收集 Python DLLs/PDBs
   - 存储到目标属性 `INTERFACE_CORONA_RUNTIME_DEPS`

2. **构建后** (`corona_install_runtime_deps`):
   - 读取依赖列表
   - 复制到可执行文件目录
   - 使用 Python 脚本增量复制（仅复制修改文件）

#### 函数参考

##### `corona_configure_runtime_deps(target_name)`

收集运行时依赖并存储到目标属性。

**调用位置**: 在 `src/CMakeLists.txt` 中对 `CoronaEngine` 目标调用一次

**示例**:
```cmake
add_library(CoronaEngine STATIC ${SOURCE_FILES})
# ... 配置目标 ...
corona_configure_runtime_deps(CoronaEngine)
```

##### `corona_install_runtime_deps(target_name)`

为可执行目标添加 POST_BUILD 复制命令。

**调用位置**: 对每个需要运行的可执行目标调用

**示例**:
```cmake
add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE CoronaEngine)
corona_install_runtime_deps(MyApp)
```

---

### 6. build_cabbage_editor.cmake

**职责**: 收集和安装编辑器资源（Backend/Frontend 目录）

#### 函数参考

##### `corona_configure_corona_editor(target_name)`

收集编辑器目录并存储到目标属性 `INTERFACE_CORONA_EDITOR_DIRS`。

##### `corona_install_corona_editor(target_name core_target)`

复制编辑器资源到可执行文件目录。

**示例**:
```cmake
# 配置阶段
corona_configure_corona_editor(CoronaEngine)

# 为可执行目标安装
add_executable(MyEditor main.cpp)
corona_install_corona_editor(MyEditor CoronaEngine)
```

---

## 添加新组件

### 添加 Utility 模块

Utility 是独立的可复用库（如 Logger、ResourceManager）。

#### 1. 创建目录结构

```
src/utility/my_module/
├── CMakeLists.txt
├── include/
│   └── MyHeader.h
└── my_impl.cpp
```

#### 2. 编写 CMakeLists.txt

```cmake
corona_add_utility(
    NAME CoronaMyModule
    TYPE STATIC                    # STATIC, SHARED, 或 INTERFACE
    DESCRIPTION "My custom utility module"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/my_impl.cpp
    PUBLIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/MyHeader.h
    DEPENDENCIES                   # 可选
        Corona::Logger
        other_lib
)

# 可选：额外配置
target_compile_definitions(CoronaMyModule PRIVATE MY_DEFINE=1)
```

#### 3. 注册模块

编辑 `src/utility/CMakeLists.txt`:

```cmake
set(_CORONA_UTILITY_MODULES
    logger
    resource_manager
    concurrent
    my_module              # 添加这行
)

set(_CORONA_UTILITY_TARGETS
    CoronaLogger
    CoronaResourceManager
    CoronaConcurrent
    CoronaMyModule         # 添加这行
)
```

#### 4. 自动生成的内容

- 构建选项: `BUILD_CORONA_MY_MODULE` (默认 ON)
- CMake 别名: `Corona::MyModule`
- 包含目录: 自动添加 `include/` 和 `../../common/include/`

---

### 添加示例程序

#### 1. 创建目录结构

```
examples/my_example/
├── CMakeLists.txt
└── my_example.cpp
```

#### 2. 编写 CMakeLists.txt

```cmake
corona_add_example(
    NAME Corona_my_example
    SOURCES my_example.cpp other_file.cpp
    COPY_ASSETS                    # 可选：复制 examples/assets/
)
```

#### 3. 注册示例

编辑 `examples/CMakeLists.txt`:

```cmake
set(_CORONA_EXAMPLE_FOLDERS
    resource_management
    # ... 其他示例 ...
    my_example              # 添加这行
)

set(_CORONA_EXAMPLE_TARGETS
    Corona_resource_management
    # ... 其他目标 ...
    Corona_my_example       # 添加这行
)
```

#### 4. 自动生成的内容

- 构建选项: `BUILD_EXAMPLE_MY_EXAMPLE` (默认 ON)
- 链接库: `CoronaEngine`, `glfw`, `CoronaConcurrent`
- 工作目录: 自动设置为可执行文件目录（VS 调试器）
- 运行时依赖: 自动复制 DLLs/PDBs

---

### 添加第三方依赖

#### 使用 FetchContent

编辑 `misc/cmake/corona_third_party.cmake`:

```cmake
FetchContent_Declare(
    my_lib
    GIT_REPOSITORY https://github.com/user/my_lib.git
    GIT_TAG v1.0.0                 # 推荐使用具体版本
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(my_lib)
message(STATUS "[3rdparty] Enabled: my_lib")
```

#### 使用本地第三方库

对于头文件库或预编译库：

```cmake
# 1. 放置到 third_party/my_lib/

# 2. 在 src/CMakeLists.txt 中添加包含目录
target_include_directories(CoronaEngine PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party/my_lib/include"
)

# 3. 如果有库文件
target_link_directories(CoronaEngine PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party/my_lib/lib"
)

target_link_libraries(CoronaEngine PUBLIC my_lib)
```

---

## CMake 函数参考

### corona_add_utility

创建 utility 库目标的统一接口。

**签名**:
```cmake
corona_add_utility(
    NAME <target_name>
    [TYPE STATIC|SHARED|INTERFACE]
    [DESCRIPTION <description>]
    [SOURCES <source_files>...]
    [PUBLIC_HEADERS <header_files>...]
    [PRIVATE_HEADERS <header_files>...]
    [DEPENDENCIES <dependencies>...]
    [INTERFACE]                     # 标志：创建 INTERFACE 库
    [HEADER_ONLY]                   # 标志：同 INTERFACE
)
```

**参数**:
- `NAME`: 目标名称（必需）
- `TYPE`: 库类型（默认 STATIC）
- `DESCRIPTION`: 描述信息
- `SOURCES`: 源文件列表
- `PUBLIC_HEADERS`: 公共头文件
- `PRIVATE_HEADERS`: 私有头文件
- `DEPENDENCIES`: 依赖库列表
- `INTERFACE` / `HEADER_ONLY`: 仅头文件库标志

**效果**:
- 创建目标
- 设置 C++20 标准
- 配置包含目录
- 创建 `Corona::TargetName` 别名
- 应用编译配置

**示例**:
```cmake
# 静态库
corona_add_utility(
    NAME CoronaUtils
    TYPE STATIC
    SOURCES utils.cpp
    PUBLIC_HEADERS include/utils.h
)

# 仅头文件库
corona_add_utility(
    NAME CoronaTemplates
    INTERFACE
    PUBLIC_HEADERS include/templates.h
)
```

---

### corona_add_example

创建示例可执行目标的统一接口。

**签名**:
```cmake
corona_add_example(
    NAME <target_name>
    SOURCES <source_files>...
    [COPY_ASSETS]                   # 标志：复制 examples/assets/
)
```

**参数**:
- `NAME`: 目标名称（必需）
- `SOURCES`: 源文件列表（必需）
- `COPY_ASSETS`: 是否复制资源目录

**自动配置**:
- 链接 `glfw`, `CoronaEngine`, `CoronaConcurrent`
- 设置 VS 调试器工作目录
- 调用 `corona_install_runtime_deps`
- 可选调用 `corona_install_corona_editor`（如果启用编辑器）
- 可选复制 `examples/assets/` 到可执行文件目录

**示例**:
```cmake
corona_add_example(
    NAME Corona_my_demo
    SOURCES main.cpp helper.cpp
    COPY_ASSETS
)
```

---

### corona_run_python

执行 Python 脚本的统一接口（在配置阶段）。

**签名**:
```cmake
corona_run_python(OUT_RESULT
    SCRIPT <script_path>
    [ARGS <arguments>...]
    [WORKING_DIRECTORY <directory>]
)
```

**输出**:
- `OUT_RESULT`: 退出码变量名
- `CORONA_LAST_PY_STDOUT`: 标准输出（自动设置）
- `CORONA_LAST_PY_STDERR`: 标准错误（自动设置）

---

### corona_to_backslash

路径斜杠转换工具。

**签名**:
```cmake
corona_to_backslash(INPUT OUT_VAR [ESCAPE_FOR_CSTRING])
```

**示例**:
```cmake
set(_unix_path "/usr/local/bin")
corona_to_backslash("${_unix_path}" _win_path)
# _win_path = "\usr\local\bin"

corona_to_backslash("${_unix_path}" _escaped ESCAPE_FOR_CSTRING)
# _escaped = "\\usr\\local\\bin" (用于 C 字符串宏)
```

---

## 依赖管理

### 第三方依赖树

```
CoronaEngine
├── assimp (模型加载)
├── EnTT (ECS)
├── CabbageHardware (平台抽象)
├── Helicon (Vulkan 渲染)
│   ├── glslang
│   ├── SPIRV-Cross
│   ├── volk
│   ├── VulkanMemoryAllocator
│   └── Vulkan-Headers
├── stb (图像加载)
├── TBB (任务并行)
├── Python 3.13+ (脚本)
└── spdlog (日志)

Examples
└── glfw (窗口管理)
```

### 依赖版本

| 依赖 | 版本 | 来源 |
|------|------|------|
| TBB | 2022.2.0 | `third_party/oneapi-tbb-2022.2.0/` |
| spdlog | 1.15.3 | `third_party/spdlog-1.15.3/` |
| Python | 3.13.7 | 系统或 `third_party/Python-3.13.7/` |
| assimp | master | FetchContent |
| entt | master | FetchContent |
| stb | master | FetchContent |
| glfw | master | FetchContent (条件) |

### 依赖更新

#### 更新 FetchContent 依赖

1. 修改 `misc/cmake/corona_third_party.cmake` 中的 `GIT_TAG`
2. 删除 `build/_deps/` 目录
3. 重新配置

```powershell
Remove-Item -Recurse -Force build/_deps/assimp-*
cmake --preset ninja-clang
```

#### 更新本地第三方库

1. 下载新版本到 `third_party/`
2. 更新 `src/CMakeLists.txt` 中的路径
3. 重新配置

---

## 常见问题

### Q: 配置失败 - Python not found

**现象**:
```
[Python] System Python (>=3.13) not found and fallback disabled
CMake Error: Interpreter not found; cannot continue
```

**解决方案**:

1. **安装 Python 3.13+** 并确保在 PATH 中
2. **指定 Python 路径**:
   ```powershell
   cmake --preset ninja-clang -DPython3_ROOT_DIR="C:\Python313"
   ```
3. **启用嵌入式 Python**（默认已启用）:
   ```powershell
   cmake --preset ninja-clang -DCORONA_PYTHON_USE_EMBEDDED_FALLBACK=ON
   ```

---

### Q: 运行时找不到 DLL

**现象**:
```
The code execution cannot proceed because tbb12.dll was not found.
```

**原因**: 运行时依赖未复制

**解决方案**:

1. **检查 CMake 配置**: 确保调用了 `corona_install_runtime_deps`
   ```cmake
   corona_install_runtime_deps(MyExecutable)
   ```

2. **重新构建**: POST_BUILD 命令仅在目标重新构建时执行
   ```powershell
   cmake --build build --config Debug --target Corona_my_example
   ```

3. **手动复制**: 临时解决
   ```powershell
   Copy-Item "third_party\oneapi-tbb-2022.2.0\redist\intel64\vc14\*.dll" `
             "build\examples\my_example\Debug\"
   ```

---

### Q: Python 依赖检查失败

**现象**:
```
[Python] Requirement check failed (exit code 1)
```

**解决方案**:

1. **启用自动安装** (默认):
   ```powershell
   cmake --preset ninja-clang -DCORONA_AUTO_INSTALL_PY_DEPS=ON
   ```

2. **手动安装**:
   ```powershell
   python -m pip install -r misc\pytools\requirements.txt
   ```

3. **跳过检查** (不推荐):
   ```powershell
   cmake --preset ninja-clang -DCORONA_CHECK_PY_DEPS=OFF
   ```

---

### Q: 编译错误 - 找不到头文件

**现象**:
```
fatal error: ResourceManager.h: No such file or directory
```

**原因**: 包含路径配置错误或依赖未链接

**解决方案**:

1. **检查目标依赖**:
   ```cmake
   target_link_libraries(MyTarget PRIVATE Corona::ResourceManager)
   ```

2. **使用正确的包含路径**:
   ```cpp
   #include <ResourceManager.h>         // utility 模块
   #include <Core/Engine/Engine.h>      // 核心引擎
   #include <Resource/Model.h>          // 资源类型
   ```

---

### Q: Visual Studio 调试时工作目录不对

**现象**: 找不到资源文件

**原因**: VS 默认工作目录是项目根目录

**解决方案**: 已自动配置

```cmake
set_target_properties(${target_name} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${target_name}>
)
```

如果仍有问题，手动设置 VS 项目属性：
```
Configuration Properties → Debugging → Working Directory = $(OutDir)
```

---

### Q: Ninja 找不到

**现象**:
```
CMake Error: CMake was unable to find a build program corresponding to "Ninja".
```

**解决方案**:

1. **安装 Ninja**:
   - Windows: `choco install ninja` 或从 [GitHub Releases](https://github.com/ninja-build/ninja/releases)
   - Linux: `sudo apt install ninja-build` (Ubuntu/Debian)
   - macOS: `brew install ninja`

2. **使用 Visual Studio 生成器** (Windows):
   ```powershell
   cmake --preset vs2022
   ```

---

## 高级用法

### 自定义编译标志

```cmake
# 添加全局编译标志
add_compile_options(-Wall -Wextra)

# 为特定目标添加
target_compile_options(CoronaEngine PRIVATE
    $<$<CONFIG:Debug>:-O0 -g>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)
```

### 条件编译

```cmake
# 平台相关
if(WIN32)
    target_compile_definitions(CoronaEngine PRIVATE PLATFORM_WINDOWS)
elseif(UNIX)
    target_compile_definitions(CoronaEngine PRIVATE PLATFORM_UNIX)
endif()

# 编译器相关
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(CoronaEngine PRIVATE -Wno-deprecated)
endif()
```

### 自定义 POST_BUILD 命令

```cmake
add_custom_command(TARGET MyTarget POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Build completed!"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/data"
        "$<TARGET_FILE_DIR:MyTarget>/data"
    COMMENT "Copying data files..."
)
```

### 导出 compile_commands.json

自动启用（所有预设包含 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`）。

用于 clangd、clang-tidy 等工具：

```powershell
# 符号链接到根目录（便于编辑器识别）
New-Item -ItemType SymbolicLink -Path "compile_commands.json" `
         -Target "build\compile_commands.json"
```

---

## 附录

### CMake 缓存变量

清除缓存重新配置：

```powershell
# 删除构建目录
Remove-Item -Recurse -Force build

# 或仅删除 CMakeCache.txt
Remove-Item build\CMakeCache.txt

# 重新配置
cmake --preset ninja-clang
```

### 调试 CMake 配置

```cmake
# 打印变量
message(STATUS "Python3_EXECUTABLE = ${Python3_EXECUTABLE}")

# 打印所有变量
get_cmake_property(_vars VARIABLES)
foreach(_var ${_vars})
    message(STATUS "${_var} = ${${_var}}")
endforeach()

# 启用详细输出
cmake --preset ninja-clang --trace
```

### 构建性能优化

```powershell
# 并行构建（自动检测 CPU 核心数）
cmake --build build --config Debug --parallel

# 指定并行数
cmake --build build --config Debug -j 8

# 仅构建特定目标
cmake --build build --config Debug --target Corona_interactive_rendering
```

---

**文档版本**: 1.0  
**最后更新**: 2025-10-05  
**引擎版本**: CoronaEngine 0.5.0  
**维护者**: CoronaEngine Team
