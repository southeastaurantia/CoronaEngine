include_guard(GLOBAL)

set(_CORONA_SYSTEM_DISPLAY_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(SystemDisplay ${_CORONA_SYSTEM_DISPLAY_DIR})

add_library(CoronaSystemDisplay STATIC)
add_library(corona::system::display ALIAS CoronaSystemDisplay)

if(CORONA_SYSTEMDISPLAY_PRIVATE_SOURCES)
    target_sources(CoronaSystemDisplay PRIVATE ${CORONA_SYSTEMDISPLAY_PRIVATE_SOURCES})
endif()

target_sources(CoronaSystemDisplay
    PUBLIC
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/systems/DisplaySystem.h
)

target_compile_features(CoronaSystemDisplay PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaSystemDisplay)

target_link_libraries(CoronaSystemDisplay
    PUBLIC
    corona::system::interface
    corona::core
)
