# =====================================================================================
# 为每个示例提供独立的子目录与构建开关
# =====================================================================================

# =====================================================================================
# 通用函数：为示例项目拷贝资源文件
# =====================================================================================
function(corona_examples_copy_assets target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "[${target_name}] Copy assets -> $<TARGET_FILE_DIR:${target_name}>/assets"
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/Examples/assets" $<TARGET_FILE_DIR:${target_name}>/assets
        VERBATIM)
endfunction()

# 引擎功能示例
option(BUILD_EXAMPLE_RESOURCE_MANAGEMENT "Build resource management example" ON)
option(BUILD_EXAMPLE_MULTI_WINDOW_RENDERING "Build multi-window rendering example" ON)
option(BUILD_EXAMPLE_PYTHON_SCRIPTING "Build Python scripting example" ON)
option(BUILD_EXAMPLE_INTERACTIVE_RENDERING "Build interactive rendering example" ON)

# 引擎功能示例
if(BUILD_EXAMPLE_RESOURCE_MANAGEMENT)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resource_management/CMakeLists.txt")
        add_subdirectory(resource_management)
    else()
        message(WARNING "[Examples] resource_management subdir not found, skip")
    endif()
endif()

if(BUILD_EXAMPLE_MULTI_WINDOW_RENDERING)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/multi_window_rendering/CMakeLists.txt")
        add_subdirectory(multi_window_rendering)
    else()
        message(WARNING "[Examples] multi_window_rendering subdir not found, skip")
    endif()
endif()

if(BUILD_EXAMPLE_PYTHON_SCRIPTING)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/python_scripting/CMakeLists.txt")
        add_subdirectory(python_scripting)
    else()
        message(WARNING "[Examples] python_scripting subdir not found, skip")
    endif()
endif()

if(BUILD_EXAMPLE_INTERACTIVE_RENDERING)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/interactive_rendering/CMakeLists.txt")
        add_subdirectory(interactive_rendering)
    else()
        message(WARNING "[Examples] interactive_rendering subdir not found, skip")
    endif()
endif()

# 选择 VS 启动项目（若对应 target 存在）
if(BUILD_EXAMPLE_RESOURCE_MANAGEMENT)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_resource_management)
elseif(BUILD_EXAMPLE_MULTI_WINDOW_RENDERING)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_multi_window_rendering)
elseif(BUILD_EXAMPLE_PYTHON_SCRIPTING)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_python_scripting)
elseif(BUILD_EXAMPLE_INTERACTIVE_RENDERING)
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Corona_interactive_rendering)
endif()
