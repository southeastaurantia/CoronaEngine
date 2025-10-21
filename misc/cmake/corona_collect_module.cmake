# ============================================================================== 
# corona_collect_module.cmake
#
# Purpose:
#   Standardize how a module (with include/src layout) collects sources and
#   public headers.
#
# Behavior:
#   - Recursively search the module `include/` directory for headers.
#   - Recursively search the module `src/` directory (or root) for sources.
#
# Outputs (uppercase variables):
#   - `CORONA_<MODULE>_PUBLIC_HEADERS`: collected public headers.
#   - `CORONA_<MODULE>_PRIVATE_SOURCES`: collected implementation sources.
#   - `CORONA_<MODULE>_ALL_FILES`: combined list of all collected files.
#
# Usage example:
#   corona_collect_module(Core "${CMAKE_CURRENT_SOURCE_DIR}/src/core")
#   add_library(CoronaCore STATIC
#       ${CORONA_CORE_PRIVATE_SOURCES}
#       ${CORONA_CORE_PUBLIC_HEADERS}
#   )
#
# Optional arguments:
#   - `QUIET`: suppress status output.
# ============================================================================== 

function(corona_collect_module MODULE_NAME MODULE_DIR)
    set(options QUIET)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(COLLECT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MODULE_NAME)
        message(FATAL_ERROR "corona_collect_module: MODULE_NAME cannot be empty")
    endif()
    if(NOT MODULE_DIR)
        message(FATAL_ERROR "corona_collect_module: MODULE_DIR cannot be empty")
    endif()

    if(NOT IS_DIRECTORY "${MODULE_DIR}")
        message(FATAL_ERROR "corona_collect_module: Module directory does not exist: ${MODULE_DIR}")
    endif()

    # Normalize the module name so derived variables use uppercase
    string(TOUPPER "${MODULE_NAME}" MODULE_NAME_UPPER)

    set(_public_dirs)
    if(IS_DIRECTORY "${MODULE_DIR}/include")
        list(APPEND _public_dirs "${MODULE_DIR}/include")
    endif()

    set(_private_dirs "${MODULE_DIR}")
    if(IS_DIRECTORY "${MODULE_DIR}/src")
        list(APPEND _private_dirs "${MODULE_DIR}/src")
    endif()
    list(REMOVE_DUPLICATES _private_dirs)

    unset(_public_headers)
    unset(_private_sources)

    foreach(_pub_dir IN LISTS _public_dirs)
        file(GLOB_RECURSE _pub_headers CONFIGURE_DEPENDS
            RELATIVE    "${MODULE_DIR}"
            "${_pub_dir}/*.h"
            "${_pub_dir}/*.hpp"
        )
        list(APPEND _public_headers ${_pub_headers})
    endforeach()

    foreach(_src_dir IN LISTS _private_dirs)
        file(GLOB_RECURSE _src_files CONFIGURE_DEPENDS
            RELATIVE    "${MODULE_DIR}"
            "${_src_dir}/*.c"
            "${_src_dir}/*.cc"
            "${_src_dir}/*.cxx"
            "${_src_dir}/*.cpp"
        )
        list(APPEND _private_sources ${_src_files})
    endforeach()

    # Convert collected relative paths back into absolute paths for the caller
    set(_public_headers_full)
    foreach(_h IN LISTS _public_headers)
        list(APPEND _public_headers_full "${MODULE_DIR}/${_h}")
    endforeach()

    set(_private_sources_full)
    foreach(_s IN LISTS _private_sources)
        list(APPEND _private_sources_full "${MODULE_DIR}/${_s}")
    endforeach()

    set(CORONA_${MODULE_NAME_UPPER}_PUBLIC_HEADERS  "${_public_headers_full}"  PARENT_SCOPE)
    set(CORONA_${MODULE_NAME_UPPER}_PRIVATE_SOURCES "${_private_sources_full}" PARENT_SCOPE)

    set(_all ${_public_headers_full} ${_private_sources_full})
    set(CORONA_${MODULE_NAME_UPPER}_ALL_FILES "${_all}" PARENT_SCOPE)

    if(NOT COLLECT_QUIET)
        list(LENGTH _public_headers_full _ph_count)
        list(LENGTH _private_sources_full _ps_count)
        message(STATUS "[Corona:Collect] ${MODULE_NAME} -> headers: ${_ph_count}, sources: ${_ps_count}")
    endif()
endfunction()

# ------------------------------------------------------------------------------
# Helper: apply shared public include directory to a target
# ------------------------------------------------------------------------------
if(NOT DEFINED CORONA_ENGINE_PUBLIC_INCLUDE_ROOT)
    set(CORONA_ENGINE_PUBLIC_INCLUDE_ROOT "${PROJECT_SOURCE_DIR}/include")
endif()

function(corona_target_use_public_includes TARGET)
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "corona_target_use_public_includes: target '${TARGET}' does not exist")
    endif()

    cmake_parse_arguments(_CORONA_INCLUDES "" "SCOPE" "" ${ARGN})

    set(_scope "${_CORONA_INCLUDES_SCOPE}")
    if(NOT _scope)
        set(_scope "PUBLIC")
    endif()

    string(TOUPPER "${_scope}" _scope_upper)
    set(_valid_scopes PUBLIC INTERFACE PRIVATE)
    list(FIND _valid_scopes "${_scope_upper}" _scope_index)
    if(_scope_index EQUAL -1)
        message(FATAL_ERROR "corona_target_use_public_includes: invalid scope '${_scope}' for target ${TARGET}")
    endif()

    target_include_directories(${TARGET}
        ${_scope_upper}
            $<BUILD_INTERFACE:${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}>
    )
endfunction()
