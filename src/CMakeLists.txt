# ==============================================================================
# CoronaEngine - 源代码目录构建配置
#
# 功能:
#   - 添加所有引擎模块子目录。
#   - 创建统一的静态库目标 `CoronaEngine`，聚合所有模块。
#   - 通过 `PUBLIC` 链接传播头文件路径和编译特性到依赖者。
# ==============================================================================

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# 添加各功能模块子目录
# ------------------------------------------------------------------------------
add_subdirectory(interfaces)     # 公共接口与对外头文件
add_subdirectory(utils)          # 通用工具类
add_subdirectory(thread)         # 线程与并发原语
add_subdirectory(systems)        # 引擎系统 (animation/audio/display/rendering)
add_subdirectory(core)           # 核心引擎逻辑 (Engine 类)
add_subdirectory(script)         # 脚本系统 (Python 集成)

# ------------------------------------------------------------------------------
# 创建 CoronaEngine 静态库目标
# ------------------------------------------------------------------------------
add_library(CoronaEngine STATIC)
add_library(corona::engine ALIAS CoronaEngine)

set_property(TARGET CoronaEngine PROPERTY OUTPUT_NAME "CoronaEngine")

# ------------------------------------------------------------------------------
# 链接所有模块目标
# ------------------------------------------------------------------------------
# 使用 PUBLIC 链接确保头文件路径和编译特性传播到依赖 CoronaEngine 的目标。
set(_corona_modules
    CoronaCore
    CoronaResource
    CoronaScriptPython
    CoronaSystemAnimation
    CoronaSystemAudio
    CoronaSystemDisplay
    CoronaSystemRendering
    CoronaThread
    CoronaLogger
    CoronaUtilityResource
    CoronaUtils
)

foreach(_module_target IN LISTS _corona_modules)
    if(TARGET ${_module_target})
        target_link_libraries(CoronaEngine PUBLIC ${_module_target})
    endif()
endforeach()

# 要求 C++20 标准
target_compile_features(CoronaEngine PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaEngine)

# 收集运行时依赖 (Python DLL/PDB) 供后续可执行目标复制
if(COMMAND corona_configure_runtime_deps)
    corona_configure_runtime_deps(CoronaEngine)
endif()

# 配置编辑器资源
if(BUILD_CORONA_EDITOR)
    corona_configure_corona_editor(CoronaEngine)
endif()
