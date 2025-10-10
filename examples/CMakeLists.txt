# 为每个示例提供独立开关并共享构建逻辑
include(CMakeParseArguments)

# 复制示例资产到可执行目录
function(corona_examples_copy_assets target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[${target_name}] Copy assets to $<TARGET_FILE_DIR:${target_name}>/assets"
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/examples/assets" $<TARGET_FILE_DIR:${target_name}>/assets
        VERBATIM)
endfunction()

# 统一的示例目标创建入口
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
    target_link_libraries(${CORONA_EX_NAME} PRIVATE glfw CoronaEngine CoronaConcurrent)

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
endfunction()

set(_CORONA_EXAMPLE_FOLDERS
    resource_management
    multi_window_rendering
    python_scripting
    interactive_rendering
    concurrent_performance
    concurrent_stability
)

set(_CORONA_EXAMPLE_TARGETS
    Corona_resource_management
    Corona_multi_window_rendering
    Corona_python_scripting
    Corona_interactive_rendering
    Corona_concurrent_performance
    Corona_concurrent_stability
)

set(_corona_startup_target "")
list(LENGTH _CORONA_EXAMPLE_FOLDERS _corona_example_count)
math(EXPR _corona_example_last "${_corona_example_count} - 1")

foreach(_corona_idx RANGE 0 ${_corona_example_last})
    list(GET _CORONA_EXAMPLE_FOLDERS ${_corona_idx} _corona_folder)
    list(GET _CORONA_EXAMPLE_TARGETS ${_corona_idx} _corona_target)
    string(TOUPPER "${_corona_folder}" _corona_upper)
    string(REPLACE "/" "_" _corona_upper "${_corona_upper}")
    string(REPLACE "-" "_" _corona_upper "${_corona_upper}")
    set(_corona_option_name "BUILD_CORONA_EXAMPLE_${_corona_upper}")
    option(${_corona_option_name} "Build ${_corona_folder} example" ON)

    if(${_corona_option_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_corona_folder}/CMakeLists.txt")
            add_subdirectory(${_corona_folder})

            if(_corona_startup_target STREQUAL "")
                set(_corona_startup_target ${_corona_target})
            endif()
        else()
            message(WARNING "[Corona:Examples] ${_corona_folder} subdir not found, skip")
        endif()
    endif()
endforeach()

if(NOT _corona_startup_target STREQUAL "")
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${_corona_startup_target})
endif()
