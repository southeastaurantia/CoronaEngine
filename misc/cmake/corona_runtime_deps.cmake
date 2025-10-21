# ============================================================================== 
# corona_runtime_deps.cmake
#
# Purpose:
#   Collect and install runtime dependencies (DLLs/PDBs) for executables.
#
# Overview:
#   1. Configure-time (`corona_configure_runtime_deps`): gather Python-related
#      runtime files and store them on the `CoronaEngine` target via the
#      `INTERFACE_CORONA_RUNTIME_DEPS` property.
#   2. Build-time (`corona_install_runtime_deps`): copy the collected files next
#      to any executable target that opts in, preserving the ability to reuse the
#      same dependency list for multiple consumers.
#
# Design highlights:
#   - Decouple collection from installation so the list can be reused.
#   - Store data on target properties instead of globals for easier extension.
#   - Keep the calls idempotent so repeated configuration updates overwrite with
#     the latest data.
# ============================================================================== 

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# Function: install runtime dependencies to a target directory
# ------------------------------------------------------------------------------
function(corona_install_runtime_deps target_name)
    # Retrieve the collected dependency list stored on the core library
    get_target_property(_CORONA_DEPS CoronaEngine INTERFACE_CORONA_RUNTIME_DEPS)

    if(NOT _CORONA_DEPS)
        message(STATUS "[Corona:RuntimeDeps] No INTERFACE_CORONA_RUNTIME_DEPS; skipping copy")
        return()
    endif()

    set(_CORONA_DESTINATION_DIR "$<TARGET_FILE_DIR:${target_name}>")

    set(_CORONA_PY_COPY "${PROJECT_SOURCE_DIR}/misc/pytools/copy_files.py")

    if(EXISTS "${_CORONA_PY_COPY}" AND DEFINED Python3_EXECUTABLE)
        set(_CORONA_DEPS_DIR "${CMAKE_BINARY_DIR}/runtime_deps")
        file(MAKE_DIRECTORY "${_CORONA_DEPS_DIR}")
        string(MD5 _corona_target_hash "${target_name}")
        set(_CORONA_DEPS_LIST "${_CORONA_DEPS_DIR}/${_corona_target_hash}.txt")
        file(WRITE "${_CORONA_DEPS_LIST}" "")

        foreach(_corona_dep_file IN LISTS _CORONA_DEPS)
            file(APPEND "${_CORONA_DEPS_LIST}" "${_corona_dep_file}\n")
        endforeach()

        add_custom_command(
            TARGET      ${target_name}
            POST_BUILD
            COMMAND     "${Python3_EXECUTABLE}" "${_CORONA_PY_COPY}" --dest "${_CORONA_DESTINATION_DIR}" --list "${_CORONA_DEPS_LIST}"
            COMMENT     "[Corona:RuntimeDeps] Copy Corona runtime dependencies to target directory -> ${target_name}"
            VERBATIM
        )
    else()
        if(NOT EXISTS "${_CORONA_PY_COPY}")
            message(STATUS "[Corona:RuntimeDeps] Python copy script not found; falling back to copy_if_different")
        else()
            message(STATUS "[Corona:RuntimeDeps] Python3 not available; falling back to copy_if_different")
        endif()

        add_custom_command(
            TARGET      ${target_name}
            POST_BUILD
            COMMAND     ${CMAKE_COMMAND} -E copy_if_different ${_CORONA_DEPS} "${_CORONA_DESTINATION_DIR}"
            COMMENT     "[Corona:RuntimeDeps] Copy runtime deps (fallback) -> ${target_name}"
            VERBATIM
        )
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Function: collect runtime dependencies during configuration
# ------------------------------------------------------------------------------
function(corona_configure_runtime_deps target_name)
    if(NOT TARGET ${target_name})
        message(WARNING "[Corona:RuntimeDeps] Target ${target_name} does not exist; cannot configure runtime dependencies.")
        return()
    endif()

    set(_CORONA_ALL_DEPS)
    if(DEFINED Python3_RUNTIME_LIBRARY_DIRS)
        file(GLOB _CORONA_PY_DLLS "${Python3_RUNTIME_LIBRARY_DIRS}/*.dll")
        file(GLOB _CORONA_PY_PDBS "${Python3_RUNTIME_LIBRARY_DIRS}/*.pdb")
        if(_CORONA_PY_DLLS)
            list(APPEND _CORONA_ALL_DEPS ${_CORONA_PY_DLLS})
        endif()
        if(_CORONA_PY_PDBS)
            list(APPEND _CORONA_ALL_DEPS ${_CORONA_PY_PDBS})
        endif()
    endif()

    if(NOT _CORONA_ALL_DEPS)
        message(WARNING "[Corona:RuntimeDeps] No runtime files collected (Python).")
        return()
    endif()

    list(REMOVE_DUPLICATES _CORONA_ALL_DEPS)
    set_target_properties(${target_name} PROPERTIES INTERFACE_CORONA_RUNTIME_DEPS "${_CORONA_ALL_DEPS}")
    message(STATUS "[Corona:RuntimeDeps] Collected ${target_name} files: ${_CORONA_ALL_DEPS}")
endfunction()
