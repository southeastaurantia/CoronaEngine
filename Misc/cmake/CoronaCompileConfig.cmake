# CoronaCompileConfig.cmake
# 编译期宏与编译器相关设置集中配置文件
# 作用：统一管理跨模块需要共享或保持一致的编译定义，减少分散定义导致的冲突与遗漏。
# 说明：使用 generator expression($<...>) 根据编译器/配置类型动态注入。

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
    # 记录当前配置解析选择的 Python 解释器路径，供运行时查询或调试输出
    CORONA_PYTHON_EXE="${Python3_EXECUTABLE}"
    CORONA_PYTHON_MODULE_DLL_DIR="${Python3_RUNTIME_LIBRARY_DIRS}/DLLs"
    CORONA_PYTHON_MODULE_LIB_DIR="${Python3_RUNTIME_LIBRARY_DIRS}/Lib"
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

# 强制 MSVC 源文件使用 UTF-8，避免中文注释/字符串在不同机器上出现编码问题
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Print a short summary of key compile definitions (non-verbose)
message(STATUS "[Compile] MSVC runtime=${CMAKE_MSVC_RUNTIME_LIBRARY}; UTF8=$<IF:$<BOOL:${MSVC}>,ON,OFF>")
