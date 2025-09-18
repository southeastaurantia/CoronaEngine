# # BuildCabbageEditor.cmake
# # Cabbage Editor resource collection and installation (mirrors CoronaRuntimeDeps pattern)
# # Overview:
# #   1. corona_configure_cabbage_editor(<core_target>) collects existing backend/frontend directories and stores them as
# #      an INTERFACE property (INTERFACE_CORONA_CABBAGE_EDITOR_DIRS) on the given target (typically CoronaEngine).
# #   2. corona_install_cabbage_editor(<executable_target>) copies those directories next to the executable at build time
# #      using a POST_BUILD custom command.
# # Design Notes:
# #   - Separation of collection & installation for reuse by multiple executables.
# #   - Uses target property instead of global variables; easy to extend later (e.g., add more editor resources).
# #   - Idempotent: re-configuring overwrites the property; install only runs for the target(s) you invoke it on.

function(corona_configure_cabbage_editor target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "[CabbageEditor] Target ${target_name} does not exist; cannot configure editor resources.")
        return()
    endif()

    # Source directories now renamed to Editor/CoronaEditor
    set(_BACKEND_DIR "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Backend")
    set(_FRONTEND_DIR "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Frontend")
    set(_EXISTING_DIRS)

    if(EXISTS "${_BACKEND_DIR}")
        list(APPEND _EXISTING_DIRS "${_BACKEND_DIR}")
    else()
        message(STATUS "[CabbageEditor] Backend directory not found: ${_BACKEND_DIR}")
    endif()

    if(EXISTS "${_FRONTEND_DIR}")
        list(APPEND _EXISTING_DIRS "${_FRONTEND_DIR}")
    else()
        message(STATUS "[CabbageEditor] Frontend directory not found: ${_FRONTEND_DIR}")
    endif()

    if(_EXISTING_DIRS)
        list(REMOVE_DUPLICATES _EXISTING_DIRS)
        set_target_properties(${target_name} PROPERTIES INTERFACE_CORONA_CABBAGE_EDITOR_DIRS "${_EXISTING_DIRS}")
        message(STATUS "[CabbageEditor] Collected directories for ${target_name}: ${_EXISTING_DIRS}")
    else()
        message(WARNING "[CabbageEditor] No editor directories collected (backend/frontend missing).")
    endif()
endfunction()

function(corona_install_cabbage_editor target_name core_target)
    if(NOT TARGET ${target_name})
        message(WARNING "[CabbageEditor] Install target ${target_name} does not exist; skipping editor resource copy.")
        return()
    endif()

    get_target_property(_EDITOR_DIRS ${core_target} INTERFACE_CORONA_CABBAGE_EDITOR_DIRS)

    if(NOT _EDITOR_DIRS)
        message(STATUS "[CabbageEditor] No collected editor directories on ${core_target}; skipping copy for ${target_name}.")
        return()
    endif()

    set(_DEST_ROOT "$<TARGET_FILE_DIR:${target_name}>/CabbageEditor")
    set(_COMMANDS)
    list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E echo "[CabbageEditor] Installing editor resources -> ${_DEST_ROOT}")
    list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory "${_DEST_ROOT}")
        set(_FRONTEND_SOURCE_DIR "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Frontend")

    foreach(_DIR IN LISTS _EDITOR_DIRS)
        get_filename_component(_BASENAME "${_DIR}" NAME)
        list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_directory "${_DIR}" "${_DEST_ROOT}/${_BASENAME}")
    endforeach()

    # Frontend npm steps moved to a standalone target defined below.

    add_custom_command(
        TARGET ${target_name} POST_BUILD
        ${_COMMANDS}
        COMMENT "[CabbageEditor] Copy editor resource directories"
        VERBATIM
    )

    # Create a separate target to handle frontend npm steps after the executable is available.
    if(BUILD_CABBAGE_EDITOR)
        set(_FRONTEND_SOURCE_DIR "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Frontend")
        if(EXISTS "${_FRONTEND_SOURCE_DIR}/package.json")
            set(_frontend_target "CabbageEditorFrontend_${target_name}")

            # Prefer bundled Node
            set(_AUTO_NODE_ROOT "${PROJECT_SOURCE_DIR}/Editor/CoronaEditor/Env/node-v22.19.0-win-x64")
            if(EXISTS "${_AUTO_NODE_ROOT}/node.exe")
                set(CORONA_NODE_ROOT "${_AUTO_NODE_ROOT}" CACHE PATH "Bundled Node root for CabbageEditor" FORCE)
                message(STATUS "[CabbageEditor] Using bundled Node at ${CORONA_NODE_ROOT}")
            endif()

            if(DEFINED CORONA_NODE_ROOT)
                set(_NODE_EXE "${CORONA_NODE_ROOT}/node.exe")
                set(_NPM_CMD "${CORONA_NODE_ROOT}/npm.cmd")
            endif()

            if(NOT DEFINED CORONA_NODE_ROOT OR NOT EXISTS "${_NODE_EXE}" OR NOT EXISTS "${_NPM_CMD}")
                message(WARNING "[CabbageEditor] Frontend target will be skipped (Node/npm not found). Set CORONA_NODE_ROOT or ensure bundled Node exists.")
            else()
                string(REPLACE ";" "\;" _ENV_PATH "$ENV{PATH}")
                set(_PATH_INJECT "PATH=${CORONA_NODE_ROOT}\;${_ENV_PATH}")

                if(EXISTS "${_FRONTEND_SOURCE_DIR}/package-lock.json")
                    set(_NPM_INSTALL_CMD_ARGS ci)
                    set(_NPM_INSTALL_ECHO "npm ci (frontend)")
                else()
                    set(_NPM_INSTALL_CMD_ARGS install --no-audit --no-fund)
                    set(_NPM_INSTALL_ECHO "npm install (frontend)")
                endif()

                add_custom_target(${_frontend_target}
                    COMMAND ${CMAKE_COMMAND} -E echo "[CabbageEditor] ${_frontend_target}: ${_NPM_INSTALL_ECHO}"
                    COMMAND ${CMAKE_COMMAND} -E chdir "${_FRONTEND_SOURCE_DIR}" ${CMAKE_COMMAND} -E env "${_PATH_INJECT}" "${_NPM_CMD}" ${_NPM_INSTALL_CMD_ARGS}
                    COMMAND ${CMAKE_COMMAND} -E echo "[CabbageEditor] ${_frontend_target}: npm run build"
                    COMMAND ${CMAKE_COMMAND} -E chdir "${_FRONTEND_SOURCE_DIR}" ${CMAKE_COMMAND} -E env "${_PATH_INJECT}" "${_NPM_CMD}" run build
                    COMMENT "[CabbageEditor] Build frontend for ${target_name}"
                    VERBATIM
                )

                # Ensure frontend target runs after the executable is built
                add_dependencies(${_frontend_target} ${target_name})
            endif()
        else()
            message(STATUS "[CabbageEditor] package.json not found, frontend target not created")
        endif()
    endif()
endfunction()