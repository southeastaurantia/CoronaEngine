# ============================================================================== 
# corona_source_groups.cmake
#
# Purpose:
#   Configure Visual Studio source groups so Solution Explorer mirrors the on-disk layout.
#
# Usage:
#   corona_setup_source_groups(<target_name>
#       [ROOT_DIR <path>]
#       [SOURCES <files...>]
#   )
#
# Behavior:
#   - Pull sources from the target automatically when the SOURCES list is omitted
#   - Map each file into a source_group TREE folder based on its real filesystem path
#   - Handle both headers and implementation files without additional setup
# ============================================================================== 

include_guard(GLOBAL)

function(corona_setup_source_groups TARGET_NAME)
    set(options "")
    set(oneValueArgs ROOT_DIR)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(SG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Default the root to the current CMakeLists folder so modules keep their local layout
    if(NOT SG_ROOT_DIR)
        set(SG_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    if(NOT IS_ABSOLUTE "${SG_ROOT_DIR}")
        get_filename_component(SG_ROOT_DIR "${SG_ROOT_DIR}" ABSOLUTE)
    endif()

    # Query the target for its sources when none were provided explicitly
    if(NOT SG_SOURCES)
        get_target_property(SG_SOURCES ${TARGET_NAME} SOURCES)
        if(NOT SG_SOURCES)
            message(WARNING "[corona_source_groups] Target '${TARGET_NAME}' has no sources")
            return()
        endif()
    endif()

    set(_abs_sources "")
    foreach(_file IN LISTS SG_SOURCES)
        if(_file MATCHES "\\$<")
            # Skip generator expressions because source_group cannot resolve them
            continue()
        endif()

        if(IS_ABSOLUTE "${_file}")
            list(APPEND _abs_sources "${_file}")
        else()
            get_filename_component(_abs "${_file}" ABSOLUTE)
            list(APPEND _abs_sources "${_abs}")
        endif()
    endforeach()

    list(REMOVE_DUPLICATES _abs_sources)

    if(NOT _abs_sources)
        return()
    endif()

    # Apply the source_group TREE mapping for every resolved file
    source_group(TREE "${SG_ROOT_DIR}" FILES ${_abs_sources})

    unset(_abs_sources)
endfunction()

function(corona_setup_system_source_groups TARGET_NAME SYSTEM_NAME)
    # Systems reuse the shared helper and rely on the default root behaviour
    corona_setup_source_groups(${TARGET_NAME})
endfunction()
