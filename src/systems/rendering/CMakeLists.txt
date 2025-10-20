include_guard(GLOBAL)

set(_CORONA_SYSTEM_RENDERING_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(SystemRendering ${_CORONA_SYSTEM_RENDERING_DIR})

add_library(CoronaSystemRendering STATIC)
add_library(corona::system::rendering ALIAS CoronaSystemRendering)

if(CORONA_SYSTEMRENDERING_PRIVATE_SOURCES)
    target_sources(CoronaSystemRendering PRIVATE ${CORONA_SYSTEMRENDERING_PRIVATE_SOURCES})
endif()

target_sources(CoronaSystemRendering
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/corona/systems/RenderingSystem.h
        ${PROJECT_SOURCE_DIR}/include/corona/systems/rendering/SceneEvents.h
)

target_compile_features(CoronaSystemRendering PUBLIC cxx_std_20)

target_include_directories(CoronaSystemRendering
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(CoronaSystemRendering
    PUBLIC
    corona::system::interface
    corona::core

    CoronaResource::Resource
    CabbageHardware
)