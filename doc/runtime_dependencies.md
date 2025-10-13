# CoronaEngine 运行时依赖管理

本文档说明 CoronaEngine 的运行时依赖收集与拷贝机制。

## 概述

CoronaEngine 使用模块化的 CMake 函数来管理不同类型的运行时依赖，确保可执行文件（examples、engine runtime）能够独立运行，无需手动拷贝依赖文件。

## 依赖类型

### 1. **Helicon/Vulkan 运行时依赖** (`helicon_install_runtime_deps`)
- **来源**: Helicon 图形库（第三方）
- **内容**: Vulkan DLLs、相关图形库 DLLs
- **调用**: 由 Helicon CMake 脚本提供
- **目标**: 所有需要图形渲染的可执行文件

### 2. **Corona 运行时依赖** (`corona_install_runtime_deps`)
- **来源**: `misc/cmake/corona_runtime_deps.cmake`
- **内容**: 
  - Python 3.13 运行时 DLLs (`python313.dll`, `vcruntime140.dll` 等)
  - Python 调试符号 PDBs (Debug 配置)
- **收集**: `corona_configure_runtime_deps(CoronaEngine)` - 在 `src/CMakeLists.txt` 中调用
- **安装**: `corona_install_runtime_deps(<target>)` - 为每个可执行目标调用
- **机制**:
  1. 配置阶段：从 `Python3_RUNTIME_LIBRARY_DIRS` 收集 DLL/PDB 文件
  2. 存储到 `CoronaEngine` 目标的 `INTERFACE_CORONA_RUNTIME_DEPS` 属性
  3. 构建后：通过 POST_BUILD 命令拷贝到可执行文件目录

### 3. **编辑器资源** (`corona_install_corona_editor`)
- **来源**: `misc/cmake/corona_editor.cmake`
- **内容**:
  - `editor/CabbageEditor/Backend/` - Python 后端脚本
  - `editor/CabbageEditor/Frontend/` - 前端资源
- **收集**: `corona_configure_corona_editor(CoronaEngine)` - 在 `src/CMakeLists.txt` 中调用
- **安装**: `corona_install_corona_editor(<target> CoronaEngine)` - 需要编辑器支持的目标调用
- **控制**: 通过 `BUILD_CORONA_EDITOR` CMake 选项控制是否启用

### 4. **Assets 资源** (自定义命令)
- **来源**: `assets/` 目录（项目根目录）
- **内容**:
  - `model/` - 3D 模型文件 (.obj, .mtl, .dae)
  - `shaders/` - GLSL 着色器 (.vert, .frag, .comp)
  - `textures/` - 纹理图片 (.png)
- **安装**: 通过 `add_custom_command` 直接拷贝目录
- **目标**: 所有需要加载资源的可执行文件

## 应用目标

### Corona_runtime (引擎主程序)
位置: `engine/CMakeLists.txt`

```cmake
# 1. Helicon 图形库依赖
helicon_install_runtime_deps(Corona_runtime)

# 2. Python 运行时依赖
corona_install_runtime_deps(Corona_runtime)

# 3. 编辑器资源 (可选)
if(BUILD_CORONA_EDITOR)
    corona_install_corona_editor(Corona_runtime CoronaEngine)
endif()

# 4. Assets 资源
add_custom_command(TARGET Corona_runtime POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${PROJECT_SOURCE_DIR}/assets" 
        "$<TARGET_FILE_DIR:Corona_runtime>/assets"
    VERBATIM
)
```

**运行时目录结构**:
```
build/engine/Debug/
├── Corona_runtime.exe
├── python313.dll
├── vcruntime140.dll
├── (其他 Helicon/Vulkan DLLs)
├── CabbageEditor/
│   ├── Backend/
│   └── Frontend/
└── assets/
    ├── model/
    ├── shaders/
    └── textures/
```

### Examples (示例程序)
位置: `examples/CMakeLists.txt`

通过 `corona_add_example()` 函数统一配置：

```cmake
function(corona_add_example)
    # ... 创建可执行文件 ...
    
    # 1. Helicon 依赖
    helicon_install_runtime_deps(${CORONA_EX_NAME})
    
    # 2. Python 运行时依赖
    corona_install_runtime_deps(${CORONA_EX_NAME})
    
    # 3. 编辑器资源 (可选)
    if(BUILD_CORONA_EDITOR)
        corona_install_corona_editor(${CORONA_EX_NAME} CoronaEngine)
    endif()
    
    # 4. Assets 资源 (可选，通过 COPY_ASSETS 选项)
    if(CORONA_EX_COPY_ASSETS)
        corona_examples_copy_assets(${CORONA_EX_NAME})
    endif()
endfunction()
```

## 设计原则

### 1. **收集与安装分离**
- **收集**：在配置阶段执行一次，结果存储在目标属性中
- **安装**：为每个需要的可执行目标单独调用，在构建后执行

### 2. **使用目标属性**
- 避免全局变量污染
- 便于扩展和维护
- 支持多个可执行目标共享同一份收集结果

### 3. **幂等性**
- 重复调用配置函数会覆盖为最新结果
- 复制命令只在构建目标后执行，不会重复执行

### 4. **灵活性**
- 通过 CMake 选项控制可选依赖（如 `BUILD_CORONA_EDITOR`）
- 支持为不同目标选择性安装依赖（如 assets 仅在需要时拷贝）

## 智能拷贝

### Python 辅助脚本
位置: `misc/pytools/copy_files.py`

使用 Python 脚本实现智能拷贝：
- 仅在文件不同时才拷贝（通过文件哈希或时间戳比较）
- 统一的日志输出格式
- 更快的批量文件处理

### 回退机制
如果 Python 不可用，回退到 CMake 的 `copy_if_different` 命令。

## 调试

### 查看收集的依赖
在 CMake 配置输出中查找：
```
[Corona:RuntimeDeps] Collected CoronaEngine files: <file list>
[Corona:Editor] Collected directories for CoronaEngine: <dir list>
```

### 查看拷贝过程
在构建输出中查找：
```
[Corona:RuntimeDeps] Copy Corona runtime dependencies to target directory -> <target>
[Corona:Editor] Installing editor resources for <target>
[Corona_runtime] Copy assets to <path>
```

### 常见问题

**Q: Python DLLs 没有被拷贝**  
A: 确保 `corona_configure_runtime_deps(CoronaEngine)` 在 `src/CMakeLists.txt` 中被调用，且 `Python3_RUNTIME_LIBRARY_DIRS` 已正确设置。

**Q: Assets 资源没有更新**  
A: 删除 `build/` 目录重新构建，或手动删除目标目录下的 `assets/` 文件夹。

**Q: 编辑器资源缺失**  
A: 确保 `BUILD_CORONA_EDITOR=ON` 且 `editor/CabbageEditor/Backend` 和 `Frontend` 目录存在。

## 扩展

### 添加新的依赖类型
1. 在 `corona_configure_runtime_deps()` 中添加收集逻辑
2. 将新依赖添加到 `_CORONA_ALL_DEPS` 列表
3. 无需修改安装函数，会自动处理

### 为新可执行文件添加依赖
```cmake
add_executable(MyNewTool main.cpp)
target_link_libraries(MyNewTool PRIVATE CoronaEngine)

# 添加所有标准依赖
helicon_install_runtime_deps(MyNewTool)
corona_install_runtime_deps(MyNewTool)

# 可选：编辑器和资源
if(BUILD_CORONA_EDITOR)
    corona_install_corona_editor(MyNewTool CoronaEngine)
endif()

# 可选：assets
add_custom_command(TARGET MyNewTool POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "${PROJECT_SOURCE_DIR}/assets" 
        "$<TARGET_FILE_DIR:MyNewTool>/assets"
)
```

## 参考

- `misc/cmake/corona_runtime_deps.cmake` - Corona 运行时依赖管理
- `misc/cmake/corona_editor.cmake` - 编辑器资源管理
- `examples/CMakeLists.txt` - 示例程序依赖配置
- `engine/CMakeLists.txt` - 引擎主程序依赖配置
