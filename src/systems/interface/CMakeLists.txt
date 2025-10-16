include_guard(GLOBAL)

set(_CORONA_SYSTEM_INTERFACE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(Utils ${_CORONA_SYSTEM_INTERFACE_DIR})

add_library(CoronaSystemInterface INTERFACE)
add_library(corona::system::interface ALIAS CoronaSystemInterface)

if(CORONA_SYSTEM_INTERFACE_PUBLIC_HEADERS)
    target_sources(CoronaSystemInterface INTERFACE ${CORONA_SYSTEM_INTERFACE_PUBLIC_HEADERS})
endif()

if(CORONA_SYSTEM_INTERFACE_PRIVATE_SOURCES)
    target_sources(CoronaSystemInterface INTERFACE ${CORONA_SYSTEM_INTERFACE_PRIVATE_SOURCES})
endif()

target_include_directories(CoronaSystemInterface
    INTERFACE
    ${_CORONA_SYSTEM_INTERFACE_DIR}/public
)

target_link_libraries(CoronaSystemInterface
    INTERFACE
    Corona::Logger
)