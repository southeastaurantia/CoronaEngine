# ==============================================================================
# CoronaEngine - 示例程序构建配置
# ==============================================================================
# 说明：
#   - 为每个示例子目录提供独立开关（BUILD_CORONA_EXAMPLE_<NAME>）
#   - 定义 corona_add_example() 函数统一配置示例程序
#   - 自动发现并添加示例子目录
#   - 设置 VS 启动项目为第一个示例
# ==============================================================================

include(CMakeParseArguments)

# ------------------------------------------------------------------------------
# 定义全局属性用于跟踪示例目标
# ------------------------------------------------------------------------------
define_property(GLOBAL PROPERTY _CORONA_EXAMPLE_TARGETS
    BRIEF_DOCS "Corona example targets"
    FULL_DOCS "List of Corona example targets created via corona_add_example")
define_property(GLOBAL PROPERTY _CORONA_EXAMPLE_REGISTRY
    BRIEF_DOCS "Corona example registry"
    FULL_DOCS "Pairs of <folder>|<target> captured when corona_add_example is invoked")
set_property(GLOBAL PROPERTY _CORONA_EXAMPLE_TARGETS "")
set_property(GLOBAL PROPERTY _CORONA_EXAMPLE_REGISTRY "")

# ------------------------------------------------------------------------------
# 工具函数：拷贝 assets 资源到示例目录
# ------------------------------------------------------------------------------

function(corona_examples_copy_assets target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[${target_name}] Copy assets to $<TARGET_FILE_DIR:${target_name}>/assets"
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" $<TARGET_FILE_DIR:${target_name}>/assets
        VERBATIM)
endfunction()

# ------------------------------------------------------------------------------
# 工具函数：添加示例程序
# ------------------------------------------------------------------------------
# 参数：
#   NAME <name>           - 示例程序名称（必需）
#   SOURCES <files...>    - 源文件列表（必需）
#   COPY_ASSETS          - 是否拷贝 assets 资源（可选标志）
# 功能：
#   - 创建可执行目标并链接 CoronaEngine
#   - 自动安装运行时依赖（Helicon/Python/Editor）
#   - 可选拷贝 assets 资源
# ------------------------------------------------------------------------------
function(corona_add_example)
    set(options COPY_ASSETS)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(CORONA_EX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CORONA_EX_NAME)
        message(FATAL_ERROR "corona_add_example requires NAME")
    endif()

    if(NOT CORONA_EX_SOURCES)
        message(FATAL_ERROR "corona_add_example requires SOURCES for ${CORONA_EX_NAME}")
    endif()

    add_executable(${CORONA_EX_NAME} ${CORONA_EX_SOURCES})
    target_compile_features(${CORONA_EX_NAME} PRIVATE cxx_std_20)
    set_target_properties(${CORONA_EX_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${CORONA_EX_NAME}>)
    target_link_libraries(${CORONA_EX_NAME} PRIVATE glfw CoronaEngine cabbage::concurrent)

    if(COMMAND helicon_install_runtime_deps)
        helicon_install_runtime_deps(${CORONA_EX_NAME})
    else()
        message(STATUS "[Corona:Example:${CORONA_EX_NAME}] helicon_install_runtime_deps not available; skip")
    endif()

    corona_install_runtime_deps(${CORONA_EX_NAME})

    if(BUILD_CORONA_EDITOR)
        corona_install_corona_editor(${CORONA_EX_NAME} CoronaEngine)
    endif()

    if(CORONA_EX_COPY_ASSETS)
        corona_examples_copy_assets(${CORONA_EX_NAME})
    endif()

    set_property(GLOBAL APPEND PROPERTY _CORONA_EXAMPLE_TARGETS ${CORONA_EX_NAME})

    if(DEFINED CORONA_CURRENT_EXAMPLE_FOLDER)
        set_property(GLOBAL APPEND PROPERTY _CORONA_EXAMPLE_REGISTRY
            "${CORONA_CURRENT_EXAMPLE_FOLDER}|${CORONA_EX_NAME}")
    endif()

    message(STATUS "[Corona:Example] ${CORONA_EX_NAME} configured")
endfunction()

# ------------------------------------------------------------------------------
# 自动发现并添加示例子目录
# ------------------------------------------------------------------------------
file(GLOB _corona_example_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
list(SORT _corona_example_dirs)
set(_corona_enabled_examples)

foreach(_corona_folder IN LISTS _corona_example_dirs)
    # 跳过非目录项
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_folder}")
        continue()
    endif()

    # 跳过 assets 目录（已移至根目录）
    if(_corona_folder STREQUAL "assets")
        continue()
    endif()

    string(TOUPPER "${_corona_folder}" _corona_upper)
    string(REPLACE "/" "_" _corona_upper "${_corona_upper}")
    string(REPLACE "-" "_" _corona_upper "${_corona_upper}")
    set(_corona_option_name "BUILD_CORONA_EXAMPLE_${_corona_upper}")
    option(${_corona_option_name} "Build ${_corona_folder} example" ON)

    if(${_corona_option_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_folder}/CMakeLists.txt")
            set(CORONA_CURRENT_EXAMPLE_FOLDER "${_corona_folder}")
            add_subdirectory(${_corona_folder})
            list(APPEND _corona_enabled_examples "${_corona_folder}")
        else()
            message(WARNING "[Corona:Examples] ${_corona_folder} subdir not found, skip")
        endif()
    endif()
endforeach()

unset(CORONA_CURRENT_EXAMPLE_FOLDER)

get_property(_corona_example_targets GLOBAL PROPERTY _CORONA_EXAMPLE_TARGETS)

if(_corona_example_targets)
    list(FILTER _corona_example_targets EXCLUDE REGEX "^$")
    list(REMOVE_DUPLICATES _corona_example_targets)
    list(LENGTH _corona_example_targets _corona_target_count)

    if(_corona_target_count GREATER 0)
        list(GET _corona_example_targets 0 _corona_startup_target)
        set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${_corona_startup_target})
    endif()
endif()

# ------------------------------------------------------------------------------
# 输出示例配置摘要
# ------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "  Corona Examples Summary")
message(STATUS "================================================================================")

if(_corona_enabled_examples)
    foreach(_corona_folder IN LISTS _corona_enabled_examples)
        message(STATUS "  ✓ ${_corona_folder}")
    endforeach()
else()
    message(STATUS "  (no examples enabled)")
endif()

get_property(_corona_example_registry GLOBAL PROPERTY _CORONA_EXAMPLE_REGISTRY)

if(_corona_example_registry)
    list(FILTER _corona_example_registry EXCLUDE REGEX "^$")
    list(REMOVE_DUPLICATES _corona_example_registry)
    set(_corona_pairs)

    foreach(_corona_entry IN LISTS _corona_example_registry)
        string(REPLACE "|" ";" _corona_split "${_corona_entry}")
        list(LENGTH _corona_split _corona_split_len)

        if(_corona_split_len EQUAL 2)
            list(GET _corona_split 0 _corona_folder_name)
            list(GET _corona_split 1 _corona_target_name)
            list(APPEND _corona_pairs "${_corona_folder_name}->${_corona_target_name}")
        else()
            list(APPEND _corona_pairs "${_corona_entry}")
        endif()
    endforeach()

    string(JOIN ", " _corona_pairs_joined ${_corona_pairs})
    message(STATUS "  Linked targets: ${_corona_pairs_joined}")
else()
    message(STATUS "  Linked targets: (none)")
endif()

message(STATUS "================================================================================")
message(STATUS "")
