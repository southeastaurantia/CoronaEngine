include_guard(GLOBAL)

set(_CORONA_UTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(Utils ${_CORONA_UTILS_DIR})

add_library(CoronaUtils INTERFACE)
add_library(corona::utils ALIAS CoronaUtils)

if(CORONA_UTILS_PUBLIC_HEADERS)
    target_sources(CoronaUtils INTERFACE ${CORONA_UTILS_PUBLIC_HEADERS})
endif()

if(CORONA_UTILS_PRIVATE_SOURCES)
    target_sources(CoronaUtils INTERFACE ${CORONA_UTILS_PRIVATE_SOURCES})
endif()

target_include_directories(CoronaUtils
    INTERFACE
        ${_CORONA_UTILS_DIR}/public
)
