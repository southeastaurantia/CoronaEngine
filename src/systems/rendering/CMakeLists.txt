include_guard(GLOBAL)

set(_CORONA_SYSTEM_RENDERING_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(SystemRendering ${_CORONA_SYSTEM_RENDERING_DIR})

add_library(CoronaSystemRendering STATIC)
add_library(corona::system::rendering ALIAS CoronaSystemRendering)

if(CORONA_SYSTEMRENDERING_PRIVATE_SOURCES)
    target_sources(CoronaSystemRendering PRIVATE ${CORONA_SYSTEMRENDERING_PRIVATE_SOURCES})
endif()

if(CORONA_SYSTEMRENDERING_PUBLIC_HEADERS)
    target_sources(CoronaSystemRendering PUBLIC ${CORONA_SYSTEMRENDERING_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaSystemRendering PUBLIC cxx_std_20)

target_include_directories(CoronaSystemRendering
    PUBLIC
    ${_CORONA_SYSTEM_RENDERING_DIR}/public
)

target_link_libraries(CoronaSystemRendering
    PUBLIC
    corona::system::interface
    corona::core
    corona::resource
    corona::utility::resource

    CabbageHardware
)