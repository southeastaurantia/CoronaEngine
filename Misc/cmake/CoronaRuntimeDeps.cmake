# CoronaRuntimeDeps.cmake
# Function to stage runtime dependencies (DLL/PDB) next to a target

function(corona_install_runtime_deps target_name)
    get_target_property(_CORONA_DEPS CoronaEngine INTERFACE_CORONA_RUNTIME_DEPS)
    if(NOT _CORONA_DEPS)
        message(STATUS "[RuntimeDeps] CoronaEngine target has no INTERFACE_CORONA_RUNTIME_DEPS defined.")
        return()
    endif()
    set(_DESTINATION_DIR "$<TARGET_FILE_DIR:${target_name}>")
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_CORONA_DEPS} "${_DESTINATION_DIR}"
        COMMENT "[RuntimeDeps] Copying Corona runtime dependencies -> ${target_name} output"
        VERBATIM
    )
endfunction()
