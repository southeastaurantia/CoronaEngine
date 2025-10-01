# Corona Utility Libraries - 统一管理和构建
# 提供类似Examples的快捷添加机制

include(CMakeParseArguments)

# 统一的Utility库创建入口
function(corona_add_utility)
    set(options INTERFACE HEADER_ONLY)
    set(oneValueArgs NAME TYPE DESCRIPTION)
    set(multiValueArgs SOURCES PUBLIC_HEADERS PRIVATE_HEADERS DEPENDENCIES)
    cmake_parse_arguments(COR_UTIL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    if(NOT COR_UTIL_NAME)
        message(FATAL_ERROR "corona_add_utility requires NAME")
    endif()
    
    # 默认类型为STATIC
    if(NOT COR_UTIL_TYPE)
        set(COR_UTIL_TYPE "STATIC")
    endif()
    
    # 根据类型创建库
    if(COR_UTIL_INTERFACE OR COR_UTIL_HEADER_ONLY)
        add_library(${COR_UTIL_NAME} INTERFACE)
        set(_lib_type "INTERFACE")
    else()
        add_library(${COR_UTIL_NAME} ${COR_UTIL_TYPE} ${COR_UTIL_SOURCES})
        set(_lib_type "PUBLIC")
    endif()
    
    # 设置C++标准
    if(NOT COR_UTIL_INTERFACE AND NOT COR_UTIL_HEADER_ONLY)
        target_compile_features(${COR_UTIL_NAME} PRIVATE cxx_std_20)
    endif()
    
    # 设置包含目录
    target_include_directories(${COR_UTIL_NAME}
        ${_lib_type}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Common/include>
            $<INSTALL_INTERFACE:include>
    )
    
    # 链接依赖
    if(COR_UTIL_DEPENDENCIES)
        target_link_libraries(${COR_UTIL_NAME} ${_lib_type} ${COR_UTIL_DEPENDENCIES})
    endif()
    
    # 应用编译器配置
    if(COMMAND corona_apply_compile_config)
        corona_apply_compile_config(${COR_UTIL_NAME})
    endif()
    
    # 创建别名
    string(REGEX REPLACE "^Corona" "Corona::" _alias_name "${COR_UTIL_NAME}")
    add_library(${_alias_name} ALIAS ${COR_UTIL_NAME})
    
    # 输出配置信息
    message(STATUS "Corona Utility [${COR_UTIL_NAME}]:")
    message(STATUS "  - Type: ${COR_UTIL_TYPE}")
    message(STATUS "  - Alias: ${_alias_name}")
    if(COR_UTIL_DESCRIPTION)
        message(STATUS "  - Description: ${COR_UTIL_DESCRIPTION}")
    endif()
endfunction()

# utility模块列表
set(_CORONA_UTILITY_MODULES
    logger
    resource_manager
    concurrent
)

# Utility目标名称列表
set(_CORONA_UTILITY_TARGETS
    CoronaLogger
    CoronaResourceManager
    CoronaConcurrent
)

# 自动为每个模块创建构建选项
list(LENGTH _CORONA_UTILITY_MODULES _utility_count)
math(EXPR _utility_last "${_utility_count} - 1")
foreach(_idx RANGE 0 ${_utility_last})
    list(GET _CORONA_UTILITY_MODULES ${_idx} _module)
    list(GET _CORONA_UTILITY_TARGETS ${_idx} _target)
    
    string(TOUPPER "${_module}" _upper)
    set(_option_name "BUILD_CORONA_${_upper}")
    
    option(${_option_name} "Build Corona ${_module} utility" ON)
    
    if(${_option_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_module}/CMakeLists.txt")
            message(STATUS "Building Corona ${_module}...")
            add_subdirectory(${_module})
        else()
            message(WARNING "[Utility] ${_module} subdir not found, skip")
        endif()
    else()
        message(STATUS "Skipping Corona ${_module} (disabled)")
    endif()
endforeach()

# 创建聚合目标（包含所有启用的Utility库）
add_library(CoronaUtilityAll INTERFACE)
set(_linked_utilities "")

foreach(_idx RANGE 0 ${_utility_last})
    list(GET _CORONA_UTILITY_MODULES ${_idx} _module)
    list(GET _CORONA_UTILITY_TARGETS ${_idx} _target)
    
    string(TOUPPER "${_module}" _upper)
    set(_option_name "BUILD_CORONA_${_upper}")
    
    if(${_option_name})
        target_link_libraries(CoronaUtilityAll INTERFACE ${_target})
        list(APPEND _linked_utilities "${_module}")
    endif()
endforeach()

# 创建聚合别名
add_library(Corona::Utility::All ALIAS CoronaUtilityAll)

# 输出构建摘要
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Corona Utility Build Summary")
message(STATUS "========================================")
foreach(_idx RANGE 0 ${_utility_last})
    list(GET _CORONA_UTILITY_MODULES ${_idx} _module)
    string(TOUPPER "${_module}" _upper)
    set(_option_name "BUILD_CORONA_${_upper}")
    
    if(${_option_name})
        message(STATUS "✓ ${_module}")
    else()
        message(STATUS "✗ ${_module} (disabled)")
    endif()
endforeach()
message(STATUS "========================================")
message(STATUS "Linked utilities: ${_linked_utilities}")
message(STATUS "========================================")
message(STATUS "")