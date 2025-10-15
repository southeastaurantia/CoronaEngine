# ==============================================================================
# corona_compile_config.cmake
#
# 功能:
#   编译期宏与编译器相关设置集中配置。
#
# 作用:
#   - 统一管理跨模块需要共享或保持一致的编译定义。
#   - 减少分散定义导致的冲突与遗漏。
#
# 说明:
#   - 使用 generator expression (`$<...>`) 根据编译器/配置类型动态注入。
#   - 包含 Python 路径宏、entt 配置、MSVC 特定设置等。
# ==============================================================================

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# C/C++ 标准设置
# ------------------------------------------------------------------------------
set(CMAKE_C_STANDARD              11)
set(CMAKE_C_STANDARD_REQUIRED     ON)
set(CMAKE_C_EXTENSIONS            ON)
set(CMAKE_CXX_STANDARD            20)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_EXTENSIONS          ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# 工具函数：路径反斜杠转换
#
# `corona_to_backslash(INPUT out_var [ESCAPE_FOR_CSTRING])`
#
# 功能:
#   将字符串中的正斜杠全部替换为反斜杠。
#
# 可选:
#   如果提供 `ESCAPE_FOR_CSTRING`，则会对反斜杠进行 C 字符串转义 (`\\` -> `\\\\`)。
# ------------------------------------------------------------------------------
function(corona_to_backslash INPUT OUT_VAR)
    set(_val "${INPUT}")

    # 先把所有正斜杠替换为反斜杠
    string(REPLACE "/" "\\" _val "${_val}")

    # 可选：转为 C 字符串中的双反斜杠
    if(ARGC GREATER 2 AND ARGV2 STREQUAL "ESCAPE_FOR_CSTRING")
        string(REPLACE "\\" "\\\\" _val "${_val}")
    endif()

    set(${OUT_VAR} "${_val}" PARENT_SCOPE)
endfunction()

# ------------------------------------------------------------------------------
# Python 路径宏定义
# ------------------------------------------------------------------------------
# 生成四个需要注入到编译宏中的 (反斜杠且 C 字符串安全的) 路径
corona_to_backslash("${Python3_EXECUTABLE}"             _CORONA_PY_EXE_ESC   ESCAPE_FOR_CSTRING)
corona_to_backslash("${Python3_RUNTIME_LIBRARY_DIRS}"   _CORONA_PY_HOME_ESC  ESCAPE_FOR_CSTRING)
corona_to_backslash("${Python3_RUNTIME_LIBRARY_DIRS}/DLLs" _CORONA_PY_DLLS_ESC ESCAPE_FOR_CSTRING)
corona_to_backslash("${Python3_RUNTIME_LIBRARY_DIRS}/Lib"  _CORONA_PY_LIB_ESC   ESCAPE_FOR_CSTRING)

# ------------------------------------------------------------------------------
# 全局编译定义
# ------------------------------------------------------------------------------
add_compile_definitions(
    # MSVC: 关闭不安全 CRT 函数警告 (如 fopen/strcpy)
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>

    # 防止 Windows 头文件定义 min/max 宏破坏 std::min/std::max
    NOMINMAX

    # EnTT 配置：指定实体/组件 ID 类型为 64 位
    ENTT_ID_TYPE=uint64_t

    # EnTT 配置：启用内部原子操作路径 (多线程安全)
    ENTT_USE_ATOMIC

    # fmt 库：强制使用 header-only 模式
    FMT_HEADER_ONLY=1

    # Python 路径宏 (用于运行时初始化)
    CORONA_PYTHON_EXE=\"${_CORONA_PY_EXE_ESC}\"
    CORONA_PYTHON_HOME_DIR=\"${_CORONA_PY_HOME_ESC}\"
    CORONA_PYTHON_MODULE_DLL_DIR=\"${_CORONA_PY_DLLS_ESC}\"
    CORONA_PYTHON_MODULE_LIB_DIR=\"${_CORONA_PY_LIB_ESC}\"

    # 根据配置类型定义调试/发布宏
    $<$<CONFIG:Debug>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:Release>:CORONA_ENGINE_RELEASE>
    $<$<CONFIG:MinSizeRel>:CORONA_ENGINE_RELEASE>
)

# ------------------------------------------------------------------------------
# MSVC 运行库策略
# ------------------------------------------------------------------------------
# 使用 /MT 或 /MTd (静态多线程运行库)，避免发布时需捆绑 VC++ 运行时 DLL
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ------------------------------------------------------------------------------
# MSVC 字符集配置
# ------------------------------------------------------------------------------
# 统一 MSVC 源/执行字符集为 UTF-8，避免与外部注入的 "/utf-8" 产生不兼容
if(MSVC)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:/source-charset:utf-8>
        $<$<COMPILE_LANGUAGE:C>:/execution-charset:utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/source-charset:utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/execution-charset:utf-8>
    )
endif()

# ------------------------------------------------------------------------------
# 配置摘要
# ------------------------------------------------------------------------------
message(STATUS "[Compile] MSVC runtime=${CMAKE_MSVC_RUNTIME_LIBRARY}")
message(STATUS "[Compile] Charset=UTF-8 (source+execution)")
