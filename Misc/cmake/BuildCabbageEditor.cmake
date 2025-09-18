# # BuildCoronaEditor.cmake
# # Corona Editor resource collection and installation (mirrors CoronaRuntimeDeps pattern)
# # Overview:
# #   1. corona_configure_corona_editor(<core_target>) collects existing backend/frontend directories and stores them as
# #      an INTERFACE property (INTERFACE_CORONA_EDITOR_DIRS) on the given target (typically CoronaEngine).
# #   2. corona_install_corona_editor(<executable_target>) copies those directories next to the executable at build time
# #      using a POST_BUILD custom command.
# # Design Notes:
# #   - Separation of collection & installation for reuse by multiple executables.
# #   - Uses target property instead of global variables; easy to extend later (e.g., add more editor resources).
# #   - Idempotent: re-configuring overwrites the property; install only runs for the target(s) you invoke it on.

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
    list(APPEND _COMMANDS COMMAND ${CMAKE_COMMAND} -E echo "[CoronaEditor] Installing editor resources -> ${_DEST_ROOT}")
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
        COMMENT "[CoronaEditor] Copy editor resource directories"
        VERBATIM
    )
endfunction()