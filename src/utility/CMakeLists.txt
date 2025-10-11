# Corona Utility Libraries - 统一管理和构建
# 提供类似Examples的快捷添加机制

include(CMakeParseArguments)

# 统一的Utility库创建入口
function(corona_add_utility)
    set(options INTERFACE HEADER_ONLY)
    set(oneValueArgs NAME TYPE DESCRIPTION)
    set(multiValueArgs SOURCES PUBLIC_HEADERS PRIVATE_HEADERS DEPENDENCIES)
    cmake_parse_arguments(CORONA_UTIL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT CORONA_UTIL_NAME)
        message(FATAL_ERROR "corona_add_utility requires NAME")
    endif()
    
    # 默认类型为STATIC
    if(NOT CORONA_UTIL_TYPE)
        set(CORONA_UTIL_TYPE "STATIC")
    endif()
    
    # 根据类型创建库
    if(CORONA_UTIL_INTERFACE OR CORONA_UTIL_HEADER_ONLY)
        add_library(${CORONA_UTIL_NAME} INTERFACE)
        set(_corona_lib_type "INTERFACE")
    else()
        add_library(${CORONA_UTIL_NAME} ${CORONA_UTIL_TYPE} ${CORONA_UTIL_SOURCES})
        set(_corona_lib_type "PUBLIC")
    endif()
    
    # 设置C++标准
    if(NOT CORONA_UTIL_INTERFACE AND NOT CORONA_UTIL_HEADER_ONLY)
        target_compile_features(${CORONA_UTIL_NAME} PRIVATE cxx_std_20)
    endif()
    
    # 设置包含目录
    target_include_directories(${CORONA_UTIL_NAME}
        ${_corona_lib_type}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../common/include>
            $<INSTALL_INTERFACE:include>
    )
    
    # 链接依赖
    if(CORONA_UTIL_DEPENDENCIES)
        target_link_libraries(${CORONA_UTIL_NAME} ${_corona_lib_type} ${CORONA_UTIL_DEPENDENCIES})
    endif()
    
    # 应用编译器配置
    if(COMMAND corona_apply_compile_config)
        corona_apply_compile_config(${CORONA_UTIL_NAME})
    endif()
    
    # 创建别名
    string(REGEX REPLACE "^Corona" "Corona::" _corona_alias_name "${CORONA_UTIL_NAME}")
    add_library(${_corona_alias_name} ALIAS ${CORONA_UTIL_NAME})
    
    # 输出配置信息
    message(STATUS "[Corona:Utility] ${CORONA_UTIL_NAME}:")
    message(STATUS "  - Type: ${CORONA_UTIL_TYPE}")
    message(STATUS "  - Alias: ${_corona_alias_name}")
    if(CORONA_UTIL_DESCRIPTION)
        message(STATUS "  - Description: ${CORONA_UTIL_DESCRIPTION}")
    endif()
endfunction()

# utility模块列表

set(_CORONA_UTILITY_MODULES
    logger
    resource_manager
)

# Utility目标名称列表
set(_CORONA_UTILITY_TARGETS
    CoronaLogger
    CoronaResourceManager
)

# 自动为每个模块创建构建选项
list(LENGTH _CORONA_UTILITY_MODULES _corona_utility_count)
math(EXPR _corona_utility_last "${_corona_utility_count} - 1")
set(_corona_built_modules "")

foreach(_corona_idx RANGE 0 ${_corona_utility_last})
    list(GET _CORONA_UTILITY_MODULES ${_corona_idx} _corona_module)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_module}/CMakeLists.txt")
        message(STATUS "[Corona:Utility] Building ${_corona_module}...")
        add_subdirectory(${_corona_module})
        list(APPEND _corona_built_modules "${_corona_module}")
    else()
        message(WARNING "[Corona:Utility] ${_corona_module} subdir not found, skip")
    endif()
endforeach()

# 创建聚合目标（包含所有已构建的Utility库）
add_library(CoronaUtilityAll INTERFACE)

foreach(_corona_idx RANGE 0 ${_corona_utility_last})
    list(GET _CORONA_UTILITY_MODULES ${_corona_idx} _corona_module)
    list(FIND _corona_built_modules ${_corona_module} _corona_built_index)
    if(_corona_built_index GREATER -1)
        list(GET _CORONA_UTILITY_TARGETS ${_corona_idx} _corona_target)
        target_link_libraries(CoronaUtilityAll INTERFACE ${_corona_target})
    endif()
endforeach()

set(_corona_linked_utilities "")
if(_corona_built_modules)
    string(JOIN ";" _corona_linked_utilities ${_corona_built_modules})
endif()

# 创建聚合别名
add_library(Corona::Utility::All ALIAS CoronaUtilityAll)

# 输出构建摘要
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Corona Utility Build Summary")
message(STATUS "========================================")
foreach(_corona_idx RANGE 0 ${_corona_utility_last})
    list(GET _CORONA_UTILITY_MODULES ${_corona_idx} _corona_module)
    list(FIND _corona_built_modules ${_corona_module} _corona_built_index)
    if(_corona_built_index GREATER -1)
        message(STATUS "✓ ${_corona_module}")
    else()
        message(STATUS "✗ ${_corona_module} (missing)")
    endif()
endforeach()
message(STATUS "========================================")
message(STATUS "Linked utilities: ${_corona_linked_utilities}")
message(STATUS "========================================")
message(STATUS "")