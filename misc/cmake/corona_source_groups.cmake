# ==============================================================================
# corona_source_groups.cmake
#
# Purpose:
#   为 Visual Studio 解决方案资源管理器提供统一的文件分组显示
#   根据文件扩展名自动将文件分类到 Header Files / Source Files / Other Files
#
# Usage:
#   corona_set_source_groups(<files...>)
#
# Example:
#   corona_set_source_groups(${MY_SOURCES} ${MY_HEADERS})
#
# Behavior:
#   - 自动识别头文件 (.h, .hpp, .hxx, .hh) -> "Header Files" 组
#   - 自动识别源文件 (.cpp, .c, .cc, .cxx) -> "Source Files" 组
#   - 其他文件 -> "Other Files" 组
#   - 保持目录层级结构（使用反斜杠分隔符以适配 VS）
# ==============================================================================

include_guard(GLOBAL)

# 主函数：根据文件路径和扩展名设置 source_group
function(corona_set_source_groups)
    # 遍历所有传入的文件
    foreach(FILE_PATH IN LISTS ARGN)
        # 获取文件的绝对路径
        get_filename_component(ABSOLUTE_PATH "${FILE_PATH}" ABSOLUTE)

        # 获取文件扩展名
        get_filename_component(FILE_EXT "${ABSOLUTE_PATH}" EXT)

        # 获取相对于项目源目录的相对路径
        file(RELATIVE_PATH REL_PATH "${PROJECT_SOURCE_DIR}" "${ABSOLUTE_PATH}")

        # 获取文件所在的目录（相对路径）
        get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)

        # 根据扩展名确定分组类型
        set(GROUP_PREFIX "")
        if(FILE_EXT MATCHES "\\.(h|hpp|hxx|hh)$")
            set(GROUP_PREFIX "Header Files")
        elseif(FILE_EXT MATCHES "\\.(cpp|c|cc|cxx)$")
            set(GROUP_PREFIX "Source Files")
        else()
            set(GROUP_PREFIX "Other Files")
        endif()

        # 如果文件在子目录中，保持目录结构
        if(REL_DIR)
            # 将路径分隔符转换为反斜杠（VS 风格）
            string(REPLACE "/" "\\" REL_DIR_VS "${REL_DIR}")
            set(FULL_GROUP "${GROUP_PREFIX}\\${REL_DIR_VS}")
        else()
            set(FULL_GROUP "${GROUP_PREFIX}")
        endif()

        # 应用 source_group
        source_group("${FULL_GROUP}" FILES "${FILE_PATH}")
    endforeach()
endfunction()

# 辅助函数：为目标自动设置 source groups
# 用法：corona_apply_source_groups_to_target(<target_name>)
function(corona_apply_source_groups_to_target TARGET_NAME)
    # 获取目标的所有源文件
    get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)

    if(TARGET_SOURCES)
        corona_set_source_groups(${TARGET_SOURCES})
    endif()
endfunction()

message(STATUS "[CMake] corona_source_groups module loaded")

