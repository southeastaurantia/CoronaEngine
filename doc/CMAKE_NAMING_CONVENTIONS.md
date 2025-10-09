# CMake 命名规范

本文档定义 CoronaEngine 项目中所有 CMake 脚本的统一命名规范。

## 一、函数命名

### 规则
- **前缀**: `corona_`（小写）
- **格式**: `corona_<动作>_<对象>` 或 `corona_<动作>`
- **风格**: snake_case

### 示例

```cmake
# ✅ 正确
function(corona_add_utility)
function(corona_add_example)
function(corona_configure_runtime_deps)
function(corona_install_runtime_deps)
function(corona_configure_corona_editor)
function(corona_install_corona_editor)
function(corona_to_backslash)

# ❌ 错误
function(COR_add_utility)           # 不应使用缩写
function(add_corona_utility)        # 前缀应在最前
function(coronaAddUtility)          # 应使用 snake_case
```

## 二、全局变量命名

### 规则
- **前缀**: `_CORONA_`（大写，下划线开头）
- **格式**: `_CORONA_<模块>_<描述>`
- **风格**: UPPER_SNAKE_CASE

### 示例

```cmake
# ✅ 正确
set(_CORONA_UTILITY_MODULES ...)
set(_CORONA_UTILITY_TARGETS ...)
set(_CORONA_EXAMPLE_FOLDERS ...)
set(_CORONA_EXAMPLE_TARGETS ...)

# ❌ 错误
set(CORONA_UTILITY_MODULES ...)     # 缺少前导下划线
set(_corona_utility_modules ...)     # 应使用大写
set(_UTIL_MODULES ...)               # 缺少 CORONA 标识
```

## 三、局部变量命名

### 规则
- **前缀**: `_corona_`（小写，下划线开头）
- **格式**: `_corona_<描述>`
- **风格**: lower_snake_case
- **作用域**: 函数内或 foreach 循环内

### 示例

```cmake
# ✅ 正确
set(_corona_option_name "BUILD_CORONA_${_corona_upper}")
set(_corona_idx 0)
set(_corona_module "logger")
set(_corona_lib_type "PUBLIC")
set(_corona_destination_dir "$<TARGET_FILE_DIR:${target_name}>")

# ❌ 错误
set(_option_name ...)                # 缺少 corona 标识
set(corona_option_name ...)          # 不应缺少前导下划线（避免污染全局）
set(_CORONA_OPTION_NAME ...)         # 局部变量应使用小写
```

## 四、函数参数命名

### 规则
- **前缀**: `CORONA_<缩写>_`（大写，函数缩写）
- **格式**: `CORONA_<缩写>_<参数名>`
- **风格**: UPPER_SNAKE_CASE
- **解析**: 通过 `cmake_parse_arguments(CORONA_<缩写> ...)` 解析

### 示例

```cmake
# ✅ 正确 - corona_add_utility 函数
cmake_parse_arguments(CORONA_UTIL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
# 生成参数: CORONA_UTIL_NAME, CORONA_UTIL_TYPE, CORONA_UTIL_SOURCES, ...

# ✅ 正确 - corona_add_example 函数
cmake_parse_arguments(CORONA_EX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
# 生成参数: CORONA_EX_NAME, CORONA_EX_SOURCES, CORONA_EX_COPY_ASSETS

# ❌ 错误
cmake_parse_arguments(COR_UTIL ...)  # 缩写不应省略 CORONA
cmake_parse_arguments(UTIL ...)      # 缺少 CORONA 前缀
```

## 五、CMake 选项命名

### 规则
- **前缀**: `BUILD_CORONA_` 或 `CORONA_`（大写）
- **格式**: `BUILD_CORONA_<模块>`（构建开关）或 `CORONA_<配置项>`（配置选项）
- **风格**: UPPER_SNAKE_CASE

### 示例

```cmake
# ✅ 正确 - 构建选项
option(BUILD_CORONA_LOGGER "Build Corona logger utility" ON)
option(BUILD_CORONA_CONCURRENT "Build Corona concurrent utility" ON)
option(BUILD_CORONA_EXAMPLE_INTERACTIVE_RENDERING "Build interactive_rendering example" ON)

# ✅ 正确 - 配置选项
option(BUILD_CORONA_EDITOR "Build Corona editor" OFF)
option(CORONA_BUILD_EXAMPLES "Build example programs" ON)
option(CORONA_CHECK_PY_DEPS "Check Python dependencies during configure" ON)
option(CORONA_AUTO_INSTALL_PY_DEPS "Auto-install missing Python packages" ON)

# ❌ 错误
option(BUILD_LOGGER ...)                          # 缺少 CORONA 标识
option(BUILD_EXAMPLE_INTERACTIVE_RENDERING ...)   # 缺少 CORONA 标识
option(CORONA_LOGGER_BUILD ...)                   # BUILD 应在最前
```

## 六、消息标签命名

### 规则
- **格式**: `[Corona:<模块>]`（方括号包裹，冒号分隔）
- **首字母大写**: Corona 和模块名首字母大写
- **用途**: 在 message() 输出中标识来源

### 示例

```cmake
# ✅ 正确
message(STATUS "[Corona:Utility] Building logger...")
message(STATUS "[Corona:Examples] ${_corona_folder} subdir not found, skip")
message(STATUS "[Corona:RuntimeDeps] Collected ${target_name} files: ${_CORONA_ALL_DEPS}")
message(WARNING "[Corona:Editor] No editor directories collected")

# ❌ 错误
message(STATUS "[Utility] Building logger...")      # 缺少 Corona 前缀
message(STATUS "[corona:utility] Building...")       # 应使用首字母大写
message(STATUS "[Corona] [Utility] Building...")     # 不应使用双层括号
message(STATUS "Corona:Utility Building...")         # 应使用方括号
```

## 七、CMake 文件命名

### 规则
- **前缀**: `corona_`（小写）
- **格式**: `corona_<功能>.cmake`
- **风格**: lower_snake_case

### 示例

```cmake
# ✅ 正确
corona_options.cmake
corona_python.cmake
corona_runtime_deps.cmake
corona_compile_config.cmake
corona_third_party.cmake
corona_editor.cmake              # (待重命名: build_cabbage_editor.cmake)

# ❌ 错误
CoronaOptions.cmake              # 应使用小写
corona-options.cmake             # 应使用下划线而非连字符
build_cabbage_editor.cmake       # 旧项目名，应重命名
options.cmake                    # 缺少 corona 前缀
```

## 八、迁移检查清单

从旧命名迁移到新规范时，检查以下项：

- [x] 所有函数使用 `corona_` 前缀
- [x] 全局变量使用 `_CORONA_` 前缀（大写）
- [x] 局部变量使用 `_corona_` 前缀（小写）
- [x] 函数参数使用 `CORONA_<缩写>_` 前缀
- [x] 构建选项使用 `BUILD_CORONA_` 前缀
- [x] 消息标签使用 `[Corona:Module]` 格式
- [x] CMake 文件使用 `corona_*.cmake` 命名
- [x] 移除所有旧项目名 `Cabbage` 引用

## 九、完整示例

### 完整的 CMake 函数示例

```cmake
# corona_add_utility 函数展示所有命名规范
function(corona_add_utility)
    # 1. 函数参数解析（CORONA_UTIL_ 前缀）
    set(options INTERFACE HEADER_ONLY)
    set(oneValueArgs NAME TYPE DESCRIPTION)
    set(multiValueArgs SOURCES PUBLIC_HEADERS PRIVATE_HEADERS DEPENDENCIES)
    cmake_parse_arguments(CORONA_UTIL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # 2. 参数校验（消息标签：[Corona:Utility]）
    if(NOT CORONA_UTIL_NAME)
        message(FATAL_ERROR "[Corona:Utility] corona_add_utility requires NAME")
    endif()
    
    # 3. 局部变量（_corona_ 前缀）
    if(NOT CORONA_UTIL_TYPE)
        set(CORONA_UTIL_TYPE "STATIC")
    endif()
    
    # 4. 条件分支（局部变量）
    if(CORONA_UTIL_INTERFACE OR CORONA_UTIL_HEADER_ONLY)
        add_library(${CORONA_UTIL_NAME} INTERFACE)
        set(_corona_lib_type "INTERFACE")
    else()
        add_library(${CORONA_UTIL_NAME} ${CORONA_UTIL_TYPE} ${CORONA_UTIL_SOURCES})
        set(_corona_lib_type "PUBLIC")
    endif()
    
    # 5. 应用配置（函数调用）
    if(COMMAND corona_apply_compile_config)
        corona_apply_compile_config(${CORONA_UTIL_NAME})
    endif()
    
    # 6. 创建别名（局部变量）
    string(REGEX REPLACE "^Corona" "Corona::" _corona_alias_name "${CORONA_UTIL_NAME}")
    add_library(${_corona_alias_name} ALIAS ${CORONA_UTIL_NAME})
    
    # 7. 输出信息（消息标签）
    message(STATUS "[Corona:Utility] ${CORONA_UTIL_NAME}:")
    message(STATUS "  - Type: ${CORONA_UTIL_TYPE}")
    message(STATUS "  - Alias: ${_corona_alias_name}")
endfunction()
```

### 完整的构建脚本示例

```cmake
# 1. 全局变量定义
set(_CORONA_UTILITY_MODULES
    logger
    resource_manager
    concurrent
)

set(_CORONA_UTILITY_TARGETS
    CoronaLogger
    CoronaResourceManager
    CoronaConcurrent
)

# 2. 自动生成选项并构建
list(LENGTH _CORONA_UTILITY_MODULES _corona_utility_count)
math(EXPR _corona_utility_last "${_corona_utility_count} - 1")

foreach(_corona_idx RANGE 0 ${_corona_utility_last})
    # 3. 循环局部变量
    list(GET _CORONA_UTILITY_MODULES ${_corona_idx} _corona_module)
    list(GET _CORONA_UTILITY_TARGETS ${_corona_idx} _corona_target)
    
    # 4. 生成选项名
    string(TOUPPER "${_corona_module}" _corona_upper)
    set(_corona_option_name "BUILD_CORONA_${_corona_upper}")
    
    # 5. 创建选项
    option(${_corona_option_name} "Build Corona ${_corona_module} utility" ON)
    
    # 6. 条件构建（消息标签）
    if(${_corona_option_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_module}/CMakeLists.txt")
            message(STATUS "[Corona:Utility] Building ${_corona_module}...")
            add_subdirectory(${_corona_module})
        else()
            message(WARNING "[Corona:Utility] ${_corona_module} subdir not found, skip")
        endif()
    endif()
endforeach()
```

## 十、参考资源

- **主 CMakeLists.txt**: `CoronaEngine/CMakeLists.txt`
- **Utility 构建脚本**: `src/utility/CMakeLists.txt`
- **Example 构建脚本**: `examples/CMakeLists.txt`
- **CMake 模块**: `misc/cmake/*.cmake`
- **构建指南**: `doc/CMAKE_BUILD_GUIDE.md`

---

**最后更新**: 2025-10-09  
**维护者**: CoronaEngine Team
