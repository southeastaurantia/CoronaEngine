# BuildCabbageEditor.cmake
# Corona Editor resource collection and installation (mirrors CoronaRuntimeDeps pattern)
# Overview:
#   1. corona_configure_corona_editor(<core_target>) collects existing backend/frontend directories and stores them as
#      an INTERFACE property (INTERFACE_CORONA_EDITOR_DIRS) on the given target (typically CoronaEngine).
#   2. corona_install_corona_editor(<executable_target>) copies those directories next to the executable at build time
#      using a POST_BUILD custom command.
# Design Notes:
#   - Separation of collection & installation for reuse by multiple executables.
#   - Uses target property instead of global variables; easy to extend later (e.g., add more editor resources).
#   - Idempotent: re-configuring overwrites the property; install only runs for the target(s) you invoke it on.

function(corona_configure_corona_editor target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "[CoronaEditor] Target ${target_name} does not exist; cannot configure editor resources.")
        return()
    endif()

    # Source directories under Editor/CoronaEditor
    set(_BACKEND_DIR "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Backend")
    set(_FRONTEND_DIR "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Frontend")
    set(_EXISTING_DIRS)

    if(EXISTS "${_BACKEND_DIR}")
        list(APPEND _EXISTING_DIRS "${_BACKEND_DIR}")
    else()
        message(STATUS "[CoronaEditor] Backend directory not found: ${_BACKEND_DIR}")
    endif()

    if(EXISTS "${_FRONTEND_DIR}")
        list(APPEND _EXISTING_DIRS "${_FRONTEND_DIR}")
    else()
        message(STATUS "[CoronaEditor] Frontend directory not found: ${_FRONTEND_DIR}")
    endif()

    if(_EXISTING_DIRS)
        list(REMOVE_DUPLICATES _EXISTING_DIRS)
        set_target_properties(${target_name} PROPERTIES INTERFACE_CORONA_EDITOR_DIRS "${_EXISTING_DIRS}")
        message(STATUS "[CoronaEditor] Collected directories for ${target_name}: ${_EXISTING_DIRS}")
    else()
        message(WARNING "[CoronaEditor] No editor directories collected (backend/frontend missing).")
    endif()
endfunction()

function(corona_install_corona_editor target_name core_target)
    if(NOT TARGET ${target_name})
        message(WARNING "[CoronaEditor] Install target ${target_name} does not exist; skipping editor resource copy.")
        return()
    endif()

    get_target_property(_EDITOR_DIRS ${core_target} INTERFACE_CORONA_EDITOR_DIRS)

    if(NOT _EDITOR_DIRS)
        message(STATUS "[CoronaEditor] No collected editor directories on ${core_target}; skipping copy for ${target_name}.")
        return()
    endif()

    set(_DEST_ROOT "$<TARGET_FILE_DIR:${target_name}>/CoronaEditor")
    set(_COMMANDS)
    list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E echo "[Editor] Install resources -> ${_DEST_ROOT}")
    list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory "${_DEST_ROOT}")

    # 上述目录复制到 <exe>/CoronaEditor 下
    # 使用 Python 脚本完成目录复制并执行 npm 构建（仅注释为中文，代码为英文）
    set(_PY_SCRIPT "${PROJECT_SOURCE_DIR}/Misc/pytools/editor_copy_and_build.py")
    set(_NODE_DIR  "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Env/node-v22.19.0-win-x64")
    set(_FRONTEND_DIR "$<TARGET_FILE_DIR:${target_name}>/CoronaEditor/Frontend")

    # 组装 --src-dir 参数列表
    set(_EDITOR_COPY_ARGS)
    foreach(_DIR IN LISTS _EDITOR_DIRS)
        list(APPEND _EDITOR_COPY_ARGS --src-dir "${_DIR}")
    endforeach()

    if(EXISTS "${_PY_SCRIPT}")
        if(DEFINED Python3_EXECUTABLE)
            list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E echo "[Editor] Sync and npm build -> ${_DEST_ROOT}")
            list(APPEND _COMMANDS COMMAND "${Python3_EXECUTABLE}" "${_PY_SCRIPT}" --dest-root "${_DEST_ROOT}" ${_EDITOR_COPY_ARGS} --frontend-dir "${_FRONTEND_DIR}" --node-dir "${_NODE_DIR}")
        else()
            message(STATUS "[CoronaEditor] Python3 executable is not available; skipping editor copy/build step.")
        endif()
    else()
        message(STATUS "[CoronaEditor] Script not found: ${_PY_SCRIPT}; skipping editor copy/build.")
    endif()

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        ${_COMMANDS}
        COMMENT "[CoronaEditor] Copy editor resource directories"
        VERBATIM
    )
endfunction()