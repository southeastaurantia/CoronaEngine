# ==============================================================================
# CoronaEngine - 源代码目录构建配置
# ==============================================================================
# 说明：
#   - 添加所有引擎模块子目录（utils/utility/thread/resource/systems/core/script）
#   - 创建统一的静态库目标 CoronaEngine，聚合所有模块
#   - 通过 PUBLIC 链接传播头文件路径和编译特性到依赖者
# ==============================================================================

include_guard(GLOBAL)

set(_CORONA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ------------------------------------------------------------------------------
# 添加各功能模块子目录
# ------------------------------------------------------------------------------
add_subdirectory(utils)      # 通用工具类
add_subdirectory(utility)    # 实用工具模块（logger/resource_manager）
add_subdirectory(thread)     # 线程与并发原语
add_subdirectory(systems)    # 引擎系统（animation/audio/display/rendering）
add_subdirectory(core)       # 核心引擎逻辑（Engine 类）
add_subdirectory(script)     # 脚本系统（Python 集成）

# ------------------------------------------------------------------------------
# 创建 CoronaEngine 静态库目标
# ------------------------------------------------------------------------------
add_library(CoronaEngine STATIC)
add_library(corona::engine ALIAS CoronaEngine)

set_property(TARGET CoronaEngine PROPERTY OUTPUT_NAME "CoronaEngine")

# ------------------------------------------------------------------------------
# 链接所有模块目标
# ------------------------------------------------------------------------------
# 使用 PUBLIC 链接确保头文件路径和编译特性传播到依赖 CoronaEngine 的目标
foreach(_module_target IN ITEMS
        CoronaCore
        CoronaResource
        CoronaScriptPython
        CoronaSystemAnimation
        CoronaSystemAudio
        CoronaSystemDisplay
        CoronaSystemRendering
        CoronaThread
        CoronaUtilityLogger
        CoronaUtilityResource
        CoronaUtils)
    if(TARGET ${_module_target})
        target_link_libraries(CoronaEngine PUBLIC ${_module_target})
    endif()
endforeach()

# 要求 C++20 标准
target_compile_features(CoronaEngine PUBLIC cxx_std_20)

if(BUILD_CORONA_EDITOR)
    corona_configure_corona_editor(CoronaEngine)
endif()
