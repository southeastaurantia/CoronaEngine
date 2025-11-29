# ==============================================================================
# corona_resource_usd.cmake
#
# Purpose:
#   Provides USD linking helper functions for CoronaResource library.
#   This file is required by the CoronaResource dependency.
#
# Functions:
#   - corona_link_usd(<target> <usd_module>): Links USD modules to a target
# ==============================================================================

include_guard(GLOBAL)

function(corona_link_usd target_name usd_module)
    if(NOT TARGET ${target_name})
        message(WARNING "[corona_link_usd] Target ${target_name} does not exist")
        return()
    endif()

    message(STATUS "corona_link_usd: 链接 ${usd_module} 到 ${target_name}")

    if(TARGET ${usd_module})
        target_link_libraries(${target_name} PRIVATE ${usd_module})
        message(STATUS "  - Linked ${usd_module} to ${target_name}")
    else()
        message(WARNING "  - USD module ${usd_module} not found, skipping")
    endif()
endfunction()
