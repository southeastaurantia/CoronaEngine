# ==============================================================================
# corona_editor.cmake
# ==============================================================================
# 功能：Corona 编辑器资源收集与安装（镜像 corona_runtime_deps 模式）
#
# 概述：
# 1. corona_configure_corona_editor(<core_target>) 收集现有的 backend/frontend
# 目录并将它们存储为 INTERFACE 属性 (INTERFACE_CORONA_EDITOR_DIRS)
# 2. corona_install_corona_editor(<executable_target>) 在构建时通过 POST_BUILD
# 自定义命令将这些目录复制到可执行文件旁边
#
# 设计要点：
# - 收集与安装分离，便于多个可执行文件重用
# - 使用目标属性而非全局变量，易于后续扩展
# - 幂等：重新配置会覆盖属性；安装仅对调用的目标运行
# ==============================================================================

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# 函数：配置阶段收集编辑器资源
# ------------------------------------------------------------------------------
function(corona_configure_corona_editor target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "[Corona:Editor] Target ${target_name} does not exist; cannot configure editor resources.")
        return()
    endif()

    # Source directories under editor/CabbageEditor
    set(_CORONA_BACKEND_DIR "${PROJECT_SOURCE_DIR}/editor/CabbageEditor/Backend")
    set(_CORONA_FRONTEND_DIR "${PROJECT_SOURCE_DIR}/editor/CabbageEditor/Frontend")
    set(_CORONA_EXISTING_DIRS)

    if(EXISTS "${_CORONA_BACKEND_DIR}")
        list(APPEND _CORONA_EXISTING_DIRS "${_CORONA_BACKEND_DIR}")
    else()
        message(STATUS "[Corona:Editor] Backend directory not found: ${_CORONA_BACKEND_DIR}")
    endif()

    if(EXISTS "${_CORONA_FRONTEND_DIR}")
        list(APPEND _CORONA_EXISTING_DIRS "${_CORONA_FRONTEND_DIR}")
    else()
        message(STATUS "[Corona:Editor] Frontend directory not found: ${_CORONA_FRONTEND_DIR}")
    endif()

    if(_CORONA_EXISTING_DIRS)
        list(REMOVE_DUPLICATES _CORONA_EXISTING_DIRS)
        set_target_properties(${target_name} PROPERTIES INTERFACE_CORONA_EDITOR_DIRS "${_CORONA_EXISTING_DIRS}")
        message(STATUS "[Corona:Editor] Collected directories for ${target_name}: ${_CORONA_EXISTING_DIRS}")
    else()
        message(WARNING "[Corona:Editor] No editor directories collected (backend/frontend missing).")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# 函数：安装编辑器资源到目标目录
# ------------------------------------------------------------------------------
function(corona_install_corona_editor target_name core_target)
    if(NOT TARGET ${target_name})
        message(WARNING "[Corona:Editor] Install target ${target_name} does not exist; skipping editor resource copy.")
        return()
    endif()

    get_target_property(_CORONA_EDITOR_DIRS ${core_target} INTERFACE_CORONA_EDITOR_DIRS)

    if(NOT _CORONA_EDITOR_DIRS)
        message(STATUS "[Corona:Editor] No collected editor directories on ${core_target}; skipping copy for ${target_name}.")
        return()
    endif()

    # 使用 Python 脚本完成目录复制并执行 npm 构建
    set(_CORONA_PY_SCRIPT "${PROJECT_SOURCE_DIR}/misc/pytools/editor_copy_and_build.py")
    set(_CORONA_NODE_DIR "${PROJECT_SOURCE_DIR}/editor/CabbageEditor/Env/node-v22.19.0-win-x64")

    if(NOT EXISTS "${_CORONA_PY_SCRIPT}")
        message(STATUS "[Corona:Editor] Script not found: ${_CORONA_PY_SCRIPT}; skipping editor copy/build.")
        return()
    endif()

    if(NOT DEFINED Python3_EXECUTABLE)
        message(STATUS "[Corona:Editor] Python3 executable is not available; skipping editor copy/build step.")
        return()
    endif()

    # 组装 --src-dir 参数
    set(_CORONA_SRC_DIR_ARGS)

    foreach(_CORONA_DIR IN LISTS _CORONA_EDITOR_DIRS)
        list(APPEND _CORONA_SRC_DIR_ARGS "--src-dir" "${_CORONA_DIR}")
    endforeach()

    # 直接使用 add_custom_command，但通过包装脚本避免命令行长度问题
    # 创建一个简单的批处理/shell 包装脚本
    if(WIN32)
        set(_CORONA_WRAPPER_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/corona_editor_install_${target_name}.bat")
        set(_SCRIPT_CONTENT "@echo off\nsetlocal\n")
        string(APPEND _SCRIPT_CONTENT "set DEST_ROOT=%~1\n")
        string(APPEND _SCRIPT_CONTENT "set FRONTEND_DIR=%~2\n")
        string(APPEND _SCRIPT_CONTENT "echo [Corona:Editor] Installing editor resources to %DEST_ROOT%\n")
        string(APPEND _SCRIPT_CONTENT "\"${Python3_EXECUTABLE}\" \"${_CORONA_PY_SCRIPT}\" --dest-root \"%DEST_ROOT%\"")

        foreach(_CORONA_DIR IN LISTS _CORONA_EDITOR_DIRS)
            string(APPEND _SCRIPT_CONTENT " --src-dir \"${_CORONA_DIR}\"")
        endforeach()

        string(APPEND _SCRIPT_CONTENT " --frontend-dir \"%FRONTEND_DIR%\" --node-dir \"${_CORONA_NODE_DIR}\"\n")
        string(APPEND _SCRIPT_CONTENT "if errorlevel 1 (\n")
        string(APPEND _SCRIPT_CONTENT "    echo [Corona:Editor] Failed to install editor resources\n")
        string(APPEND _SCRIPT_CONTENT "    exit /b 1\n")
        string(APPEND _SCRIPT_CONTENT ")\n")
        string(APPEND _SCRIPT_CONTENT "echo [Corona:Editor] Successfully installed editor resources\n")
        file(WRITE "${_CORONA_WRAPPER_SCRIPT}" "${_SCRIPT_CONTENT}")

        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND cmd /c "${_CORONA_WRAPPER_SCRIPT}" "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor" "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor/Frontend"
            COMMENT "[Corona:Editor] Installing editor resources..."
            VERBATIM
        )
    else()
        set(_CORONA_WRAPPER_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/corona_editor_install_${target_name}.sh")
        set(_SCRIPT_CONTENT "#!/bin/bash\n")
        string(APPEND _SCRIPT_CONTENT "DEST_ROOT=\"$1\"\n")
        string(APPEND _SCRIPT_CONTENT "FRONTEND_DIR=\"$2\"\n")
        string(APPEND _SCRIPT_CONTENT "echo \"[Corona:Editor] Installing editor resources to $DEST_ROOT\"\n")
        string(APPEND _SCRIPT_CONTENT "\"${Python3_EXECUTABLE}\" \"${_CORONA_PY_SCRIPT}\" --dest-root \"$DEST_ROOT\"")

        foreach(_CORONA_DIR IN LISTS _CORONA_EDITOR_DIRS)
            string(APPEND _SCRIPT_CONTENT " --src-dir \"${_CORONA_DIR}\"")
        endforeach()

        string(APPEND _SCRIPT_CONTENT " --frontend-dir \"$FRONTEND_DIR\" --node-dir \"${_CORONA_NODE_DIR}\"\n")
        string(APPEND _SCRIPT_CONTENT "if [ $? -ne 0 ]; then\n")
        string(APPEND _SCRIPT_CONTENT "    echo \"[Corona:Editor] Failed to install editor resources\"\n")
        string(APPEND _SCRIPT_CONTENT "    exit 1\n")
        string(APPEND _SCRIPT_CONTENT "fi\n")
        string(APPEND _SCRIPT_CONTENT "echo \"[Corona:Editor] Successfully installed editor resources\"\n")
        file(WRITE "${_CORONA_WRAPPER_SCRIPT}" "${_SCRIPT_CONTENT}")

        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND bash "${_CORONA_WRAPPER_SCRIPT}" "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor" "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor/Frontend"
            COMMENT "[Corona:Editor] Installing editor resources..."
            VERBATIM
        )
    endif()
endfunction()