# ==============================================================================
# corona_collect_module.cmake
#
# 功能:
#   标准化收集单个模块 (include/src 布局) 的源文件和头文件。
#
# 规则:
#   - 递归搜索模块 `include/` 目录下的所有头文件。
#   - 递归搜索模块 `src/` 目录下的所有实现文件。
#
# 生成变量 (全部大写):
#   - `CORONA_<MODULE>_PUBLIC_HEADERS`: 公共头文件列表。
#   - `CORONA_<MODULE>_PRIVATE_SOURCES`: 私有源文件列表。
#   - `CORONA_<MODULE>_ALL_FILES`: 所有文件聚合列表。
#
# 使用示例:
#   corona_collect_module(Core "${CMAKE_CURRENT_SOURCE_DIR}/src/core")
#   add_library(CoronaCore STATIC
#       ${CORONA_CORE_PRIVATE_SOURCES}
#       ${CORONA_CORE_PUBLIC_HEADERS}
#   )
#
# 可选参数:
#   - `QUIET`: 关闭收集结果输出。
# ==============================================================================

function(corona_collect_module MODULE_NAME MODULE_DIR)
    set(options QUIET)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(COLLECT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MODULE_NAME)
        message(FATAL_ERROR "corona_collect_module: MODULE_NAME 不能为空")
    endif()
    if(NOT MODULE_DIR)
        message(FATAL_ERROR "corona_collect_module: MODULE_DIR 不能为空")
    endif()

    if(NOT IS_DIRECTORY "${MODULE_DIR}")
        message(FATAL_ERROR "corona_collect_module: 模块目录不存在: ${MODULE_DIR}")
    endif()

    # 规范化模块名: 变量使用大写
    string(TOUPPER "${MODULE_NAME}" MODULE_NAME_UPPER)

    set(_public_dirs)
    if(IS_DIRECTORY "${MODULE_DIR}/include")
        list(APPEND _public_dirs "${MODULE_DIR}/include")
    endif()

    set(_private_dirs "${MODULE_DIR}")
    if(IS_DIRECTORY "${MODULE_DIR}/src")
        list(APPEND _private_dirs "${MODULE_DIR}/src")
    endif()
    list(REMOVE_DUPLICATES _private_dirs)

    # 递归收集所有文件
    unset(_public_headers)
    unset(_private_sources)

    foreach(_pub_dir IN LISTS _public_dirs)
        file(GLOB_RECURSE _pub_headers CONFIGURE_DEPENDS
            RELATIVE    "${MODULE_DIR}"
            "${_pub_dir}/*.h"
            "${_pub_dir}/*.hpp"
        )
        list(APPEND _public_headers ${_pub_headers})
    endforeach()

    foreach(_src_dir IN LISTS _private_dirs)
        file(GLOB_RECURSE _src_files CONFIGURE_DEPENDS
            RELATIVE    "${MODULE_DIR}"
            "${_src_dir}/*.c"
            "${_src_dir}/*.cc"
            "${_src_dir}/*.cxx"
            "${_src_dir}/*.cpp"
        )
        list(APPEND _private_sources ${_src_files})
    endforeach()

    # 前面使用 RELATIVE 生成的相对路径基于 MODULE_DIR，加上前缀返回给调用者
    set(_public_headers_full)
    foreach(_h IN LISTS _public_headers)
        list(APPEND _public_headers_full "${MODULE_DIR}/${_h}")
    endforeach()

    set(_private_sources_full)
    foreach(_s IN LISTS _private_sources)
        list(APPEND _private_sources_full "${MODULE_DIR}/${_s}")
    endforeach()

    # 导出变量
    set(CORONA_${MODULE_NAME_UPPER}_PUBLIC_HEADERS  "${_public_headers_full}"  PARENT_SCOPE)
    set(CORONA_${MODULE_NAME_UPPER}_PRIVATE_SOURCES "${_private_sources_full}" PARENT_SCOPE)

    set(_all ${_public_headers_full} ${_private_sources_full})
    set(CORONA_${MODULE_NAME_UPPER}_ALL_FILES "${_all}" PARENT_SCOPE)

    if(NOT COLLECT_QUIET)
        list(LENGTH _public_headers_full _ph_count)
        list(LENGTH _private_sources_full _ps_count)
        message(STATUS "[Corona:Collect] ${MODULE_NAME} -> headers: ${_ph_count}, sources: ${_ps_count}")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Helper: apply shared public include directory to a target
# ------------------------------------------------------------------------------
if(NOT DEFINED CORONA_ENGINE_PUBLIC_INCLUDE_ROOT)
    set(CORONA_ENGINE_PUBLIC_INCLUDE_ROOT "${PROJECT_SOURCE_DIR}/include")
endif()

function(corona_target_use_public_includes TARGET)
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "corona_target_use_public_includes: target '${TARGET}' does not exist")
    endif()

    cmake_parse_arguments(_CORONA_INCLUDES "" "SCOPE" "" ${ARGN})

    set(_scope "${_CORONA_INCLUDES_SCOPE}")
    if(NOT _scope)
        set(_scope "PUBLIC")
    endif()

    string(TOUPPER "${_scope}" _scope_upper)
    set(_valid_scopes PUBLIC INTERFACE PRIVATE)
    list(FIND _valid_scopes "${_scope_upper}" _scope_index)
    if(_scope_index EQUAL -1)
        message(FATAL_ERROR "corona_target_use_public_includes: invalid scope '${_scope}' for target ${TARGET}")
    endif()

    target_include_directories(${TARGET}
        ${_scope_upper}
            $<BUILD_INTERFACE:${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}>
    )
endfunction()
