# 为每个示例提供独立开关并共享构建逻辑
include(CMakeParseArguments)

# 复制示例资产到可执行目录
function(corona_examples_copy_assets target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[${target_name}] Copy assets -> $<TARGET_FILE_DIR:${target_name}>/assets"
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Examples/assets" $<TARGET_FILE_DIR:${target_name}>/assets
        VERBATIM)
endfunction()

# 统一的示例目标创建入口
function(corona_add_example)
    set(options COPY_ASSETS)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(COR_EX "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT COR_EX_NAME)
        message(FATAL_ERROR "corona_add_example requires NAME")
    endif()
    if(NOT COR_EX_SOURCES)
        message(FATAL_ERROR "corona_add_example requires SOURCES for ${COR_EX_NAME}")
    endif()

    add_executable(${COR_EX_NAME} ${COR_EX_SOURCES})
    target_compile_features(${COR_EX_NAME} PRIVATE cxx_std_20)
    set_target_properties(${COR_EX_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${COR_EX_NAME}>)
    target_link_libraries(${COR_EX_NAME} PRIVATE glfw CoronaEngine)

    if(COMMAND helicon_install_runtime_deps)
        helicon_install_runtime_deps(${COR_EX_NAME})
    else()
        message(STATUS "[${COR_EX_NAME}] helicon_install_runtime_deps not available; skip")
    endif()
    corona_install_runtime_deps(${COR_EX_NAME})
    if(BUILD_CORONA_EDITOR)
        corona_install_corona_editor(${COR_EX_NAME} CoronaEngine)
    endif()

    if(COR_EX_COPY_ASSETS)
        corona_examples_copy_assets(${COR_EX_NAME})
    endif()
endfunction()

set(_CORONA_EXAMPLE_FOLDERS
    concurrent_queue_usage
    concurrent_queue_performance
    resource_management
    multi_window_rendering
    python_scripting
    interactive_rendering
)

set(_CORONA_EXAMPLE_TARGETS
    Corona_concurrent_queue_usage
    Corona_concurrent_queue_performance
    Corona_resource_management
    Corona_multi_window_rendering
    Corona_python_scripting
    Corona_interactive_rendering
)

set(_corona_startup_target "")
list(LENGTH _CORONA_EXAMPLE_FOLDERS _corona_example_count)
math(EXPR _corona_example_last "${_corona_example_count} - 1")
foreach(_idx RANGE 0 ${_corona_example_last})
    list(GET _CORONA_EXAMPLE_FOLDERS ${_idx} _folder)
    list(GET _CORONA_EXAMPLE_TARGETS ${_idx} _target)
    string(TOUPPER "${_folder}" _upper)
    string(REPLACE "/" "_" _upper "${_upper}")
    string(REPLACE "-" "_" _upper "${_upper}")
    set(_option_name "BUILD_EXAMPLE_${_upper}")
    option(${_option_name} "Build ${_folder} example" ON)
    if(${_option_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_folder}/CMakeLists.txt")
            add_subdirectory(${_folder})
            if(_corona_startup_target STREQUAL "")
                set(_corona_startup_target ${_target})
            endif()
        else()
            message(WARNING "[Examples] ${_folder} subdir not found, skip")
        endif()
    endif()
endforeach()

if(NOT _corona_startup_target STREQUAL "")
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${_corona_startup_target})
endif()
