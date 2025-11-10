# ==============================================================================
# CoronaEngine - Source Library
#
# 构建引擎核心库和系统模块
# ==============================================================================

# ------------------------------------------------------------------------------
# 自动发现并添加系统模块
# ------------------------------------------------------------------------------
message(STATUS "[CoronaEngine] Discovering system modules...")

file(GLOB _CORONA_SYSTEM_DIRS LIST_DIRECTORIES true 
     "${CMAKE_CURRENT_SOURCE_DIR}/systems/src/*")

foreach(_system_dir IN LISTS _CORONA_SYSTEM_DIRS)
    if(IS_DIRECTORY "${_system_dir}")
        get_filename_component(_system_name "${_system_dir}" NAME)
        
        if(EXISTS "${_system_dir}/CMakeLists.txt")
            add_subdirectory("systems/src/${_system_name}")
        else()
            message(WARNING "[CoronaEngine] System '${_system_name}' has no CMakeLists.txt, skipping")
        endif()
    endif()
endforeach()

unset(_CORONA_SYSTEM_DIRS)
unset(_system_dir)
unset(_system_name)

# ------------------------------------------------------------------------------
# 添加脚本模块
# ------------------------------------------------------------------------------
add_subdirectory(script/src)

# ==============================================================================
# CoronaEngine 核心库
# ==============================================================================

add_library(CoronaEngine STATIC
    engine.cpp
    shared_data_hub.cpp

  include/corona/engine.h
    include/corona/shared_data_hub.h
)

add_library(corona::engine ALIAS CoronaEngine)


# 设置包含目录
target_include_directories(CoronaEngine PUBLIC
    ${PROJECT_SOURCE_DIR}/src/include
)

# 链接依赖
target_link_libraries(CoronaEngine PUBLIC
    corona::kernel
    corona::system::acoustics
    corona::system::optics
    corona::system::mechanics
    corona::system::geometry
    corona::system::animation
    corona::system::display
    corona::script::python
)

# 设置 C++ 标准
target_compile_features(CoronaEngine PUBLIC cxx_std_20)

# 配置运行时依赖（Python DLLs/PDBs 等）
# 这会将依赖文件列表存储在 CoronaEngine 目标的 INTERFACE_CORONA_RUNTIME_DEPS 属性中
# 供消费者（如 examples）通过 corona_install_runtime_deps() 安装
corona_configure_runtime_deps(CoronaEngine)

# 配置编辑器资源
if(BUILD_CORONA_EDITOR)
    corona_configure_corona_editor(CoronaEngine)
endif()

message(STATUS "[CoronaEngine] Core engine library configured")
