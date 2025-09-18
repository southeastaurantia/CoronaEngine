# CoronaCompileConfig.cmake
# 编译期宏与编译器相关设置集中配置文件
# 作用：统一管理跨模块需要共享或保持一致的编译定义，减少分散定义导致的冲突与遗漏。
# 说明：使用 generator expression($<...>) 根据编译器/配置类型动态注入。

## --- Normalize Python paths for C string literals (avoid backslash escapes) ---
set(_CORONA_PY_EXE "${Python3_EXECUTABLE}")
file(TO_CMAKE_PATH "${_CORONA_PY_EXE}" _CORONA_PY_EXE_FWD)

set(_CORONA_PY_RT_DIR_RAW "${Python3_RUNTIME_LIBRARY_DIRS}")
# If it's a list, take the first element as primary runtime dir
if(_CORONA_PY_RT_DIR_RAW)
    if(_CORONA_PY_RT_DIR_RAW MATCHES ";")
        list(GET _CORONA_PY_RT_DIR_RAW 0 _CORONA_PY_RT_DIR)
    else()
        set(_CORONA_PY_RT_DIR "${_CORONA_PY_RT_DIR_RAW}")
    endif()
    file(TO_CMAKE_PATH "${_CORONA_PY_RT_DIR}" _CORONA_PY_RT_DIR_FWD)
endif()

# --- If user requests backslashes, produce C string literals with escaped backslashes ---
# Convert forward paths to backslash form and then escape backslashes (e.g., C:\\Path\\to\\file)
set(_CORONA_PY_EXE_BS "${_CORONA_PY_EXE_FWD}")
string(REPLACE "/" "\\" _CORONA_PY_EXE_BS "${_CORONA_PY_EXE_BS}")
string(REPLACE "\\" "\\\\" _CORONA_PY_EXE_ESC "${_CORONA_PY_EXE_BS}")

if(DEFINED _CORONA_PY_RT_DIR_FWD)
    set(_CORONA_PY_DLLS_FWD "${_CORONA_PY_RT_DIR_FWD}/DLLs")
    set(_CORONA_PY_LIB_FWD   "${_CORONA_PY_RT_DIR_FWD}/Lib")

    set(_CORONA_PY_DLLS_BS "${_CORONA_PY_DLLS_FWD}")
    set(_CORONA_PY_LIB_BS   "${_CORONA_PY_LIB_FWD}")
    string(REPLACE "/" "\\" _CORONA_PY_DLLS_BS "${_CORONA_PY_DLLS_BS}")
    string(REPLACE "/" "\\" _CORONA_PY_LIB_BS   "${_CORONA_PY_LIB_BS}")
    string(REPLACE "\\" "\\\\" _CORONA_PY_DLLS_ESC "${_CORONA_PY_DLLS_BS}")
    string(REPLACE "\\" "\\\\" _CORONA_PY_LIB_ESC   "${_CORONA_PY_LIB_BS}")
endif()

add_compile_definitions(
    # (MSVC) 关闭 MSVC 对不安全 CRT 函数的警告（如 fopen/strcpy），保证日志整洁
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    # 防止 Windows 头文件定义 min/max 宏破坏 std::min/std::max
    NOMINMAX
    # 指定 entt 的实体/组件 ID 类型为 64 位，提供更大空间 (避免默认变化带来的 ABI 差异)
    ENTT_ID_TYPE=uint64_t
    # 启用 entt 内部的原子操作路径（在多线程场景下更安全）
    ENTT_USE_ATOMIC
    # 强制 fmt 使用 header-only 模式，避免单独编译库（适合轻量集成）
    FMT_HEADER_ONLY=1
    # 记录 Python 可执行与运行库路径（反斜杠样式，已按 C 字符串转义）
    CORONA_PYTHON_EXE="${_CORONA_PY_EXE_ESC}"
    $<$<BOOL:${_CORONA_PY_DLLS_ESC}>:CORONA_PYTHON_MODULE_DLL_DIR="${_CORONA_PY_DLLS_ESC}">
    $<$<BOOL:${_CORONA_PY_LIB_ESC}>:CORONA_PYTHON_MODULE_LIB_DIR="${_CORONA_PY_LIB_ESC}">
    # Debug / RelWithDebInfo 统一定义调试宏；Release / MinSizeRel 定义发布宏
    $<$<CONFIG:Debug>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:Release>:CORONA_ENGINE_RELEASE>
    $<$<CONFIG:MinSizeRel>:CORONA_ENGINE_RELEASE>
)

## MSVC 运行库策略
# 使用 /MT 或 /MTd (静态多线程运行库)，避免最终发布目录还需捆绑 VC++ 运行时 DLL。
# 如需与外部动态库或插件系统共享 CRT，可切换为 MultiThreadedDLL 变体。
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 统一 MSVC 源/执行字符集为 UTF-8，避免与外部注入的 "/utf-8" 或 
# "/source-charset:utf-8" 产生不兼容组合（两者不能同时存在）。
# 采用更细粒度的等价设置：/source-charset:utf-8 与 /execution-charset:utf-8。
if(MSVC)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:/source-charset:utf-8>
        $<$<COMPILE_LANGUAGE:C>:/execution-charset:utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/source-charset:utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/execution-charset:utf-8>
    )
endif()

# Print a short summary of key compile definitions (non-verbose)
message(STATUS "[Compile] MSVC runtime=${CMAKE_MSVC_RUNTIME_LIBRARY}; charset=UTF-8($<IF:$<BOOL:${MSVC}>,source+execution,off>)")
