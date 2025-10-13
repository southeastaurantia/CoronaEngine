# ==============================================================================
# corona_collect_module.cmake
# ==============================================================================
# 功能：标准化收集单个模块（public/private 分层）的源文件和头文件
#
# 规则：
#   - 仅在模块根目录下非递归收集
#   - public 目录：收集 *.h *.hpp (导出头文件)
#   - private 目录：收集 *.c *.cc *.cxx *.cpp (实现文件)
#
# 生成变量（全部大写）：
#   CORONA_<MODULE>_PUBLIC_HEADERS  - 公共头文件列表
#   CORONA_<MODULE>_PRIVATE_SOURCES - 私有源文件列表
#   CORONA_<MODULE>_ALL_FILES       - 所有文件聚合列表
#
# 使用示例：
#   corona_collect_module(Core "${CMAKE_CURRENT_SOURCE_DIR}/src/core")
#   add_library(CoronaCore STATIC 
#       ${CORONA_CORE_PRIVATE_SOURCES} 
#       ${CORONA_CORE_PUBLIC_HEADERS}
#   )
#
# 可选参数：
#   QUIET - 关闭收集结果输出
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

    set(PUBLIC_DIR "${MODULE_DIR}/public")
    set(PRIVATE_DIR "${MODULE_DIR}/private")

    # 仅非递归收集
    unset(_public_headers)
    unset(_private_sources)

    if(IS_DIRECTORY "${PUBLIC_DIR}")
        file(GLOB _public_headers CONFIGURE_DEPENDS
            RELATIVE "${MODULE_DIR}"
            "${PUBLIC_DIR}/*.h"
            "${PUBLIC_DIR}/*.hpp"
        )
    endif()

    if(IS_DIRECTORY "${PRIVATE_DIR}")
        file(GLOB _private_sources CONFIGURE_DEPENDS
            RELATIVE "${MODULE_DIR}"
            "${PRIVATE_DIR}/*.c"
            "${PRIVATE_DIR}/*.cc"
            "${PRIVATE_DIR}/*.cxx"
            "${PRIVATE_DIR}/*.cpp"
        )
    endif()

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
    set(CORONA_${MODULE_NAME_UPPER}_PUBLIC_HEADERS ${_public_headers_full} PARENT_SCOPE)
    set(CORONA_${MODULE_NAME_UPPER}_PRIVATE_SOURCES ${_private_sources_full} PARENT_SCOPE)

    set(_all ${_public_headers_full} ${_private_sources_full})
    set(CORONA_${MODULE_NAME_UPPER}_ALL_FILES ${_all} PARENT_SCOPE)

    if(NOT COLLECT_QUIET)
        list(LENGTH _public_headers_full _ph_count)
        list(LENGTH _private_sources_full _ps_count)
        message(STATUS "[Corona:Collect] ${MODULE_NAME} -> public: ${_ph_count}, private: ${_ps_count}")
    endif()
endfunction()
