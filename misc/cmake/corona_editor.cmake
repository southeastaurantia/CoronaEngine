# corona_editor.cmake
# Corona Editor resource collection and installation (mirrors corona_runtime_deps pattern)
# Overview:
include_guard(GLOBAL)

# 1. corona_configure_corona_editor(<core_target>) collects existing backend/frontend directories and stores them as
# an INTERFACE property (INTERFACE_CORONA_EDITOR_DIRS) on the given target (typically CoronaEngine).
# 2. corona_install_corona_editor(<executable_target>) copies those directories next to the executable at build time
# using a POST_BUILD custom command.
# Design Notes:
# - Separation of collection & installation for reuse by multiple executables.
# - Uses target property instead of global variables; easy to extend later (e.g., add more editor resources).
# - Idempotent: re-configuring overwrites the property; install only runs for the target(s) you invoke it on.
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

    set(_CORONA_DEST_ROOT "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor")
    set(_CORONA_COMMANDS)
    list(APPEND _CORONA_COMMANDS COMMAND ${CMAKE_COMMAND} -E echo "[Corona:Editor] Install resources -> ${_CORONA_DEST_ROOT}")
    list(APPEND _CORONA_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory "${_CORONA_DEST_ROOT}")

    # 上述目录复制到 <exe>/CabbageEditor 下
    # 使用 Python 脚本完成目录复制并执行 npm 构建（仅注释为中文，代码为英文）
    set(_CORONA_PY_SCRIPT "${PROJECT_SOURCE_DIR}/misc/pytools/editor_copy_and_build.py")
    set(_CORONA_NODE_DIR "${PROJECT_SOURCE_DIR}/editor/CabbageEditor/Env/node-v22.19.0-win-x64")
    set(_CORONA_FRONTEND_DIR "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor/Frontend")

    # 组装 --src-dir 参数列表
    set(_CORONA_EDITOR_COPY_ARGS)

    foreach(_CORONA_DIR IN LISTS _CORONA_EDITOR_DIRS)
        list(APPEND _CORONA_EDITOR_COPY_ARGS --src-dir "${_CORONA_DIR}")
    endforeach()

    if(EXISTS "${_CORONA_PY_SCRIPT}")
        if(DEFINED Python3_EXECUTABLE)
            list(APPEND _CORONA_COMMANDS COMMAND ${CMAKE_COMMAND} -E echo "[Corona:Editor] Sync and npm build -> ${_CORONA_DEST_ROOT}")
            list(APPEND _CORONA_COMMANDS COMMAND "${Python3_EXECUTABLE}" "${_CORONA_PY_SCRIPT}" --dest-root "${_CORONA_DEST_ROOT}" ${_CORONA_EDITOR_COPY_ARGS} --frontend-dir "${_CORONA_FRONTEND_DIR}" --node-dir "${_CORONA_NODE_DIR}")
        else()
            message(STATUS "[Corona:Editor] Python3 executable is not available; skipping editor copy/build step.")
        endif()
    else()
        message(STATUS "[Corona:Editor] Script not found: ${_CORONA_PY_SCRIPT}; skipping editor copy/build.")
    endif()

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        ${_CORONA_COMMANDS}
        COMMENT "[Corona:Editor] Copy editor resource directories"
        VERBATIM
    )
endfunction()