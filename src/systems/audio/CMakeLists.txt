include_guard(GLOBAL)

set(_CORONA_SYSTEM_AUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(SystemAudio ${_CORONA_SYSTEM_AUDIO_DIR})

add_library(CoronaSystemAudio STATIC)
add_library(corona::system::audio ALIAS CoronaSystemAudio)

if(CORONA_SYSTEMAUDIO_PRIVATE_SOURCES)
    target_sources(CoronaSystemAudio PRIVATE ${CORONA_SYSTEMAUDIO_PRIVATE_SOURCES})
endif()

target_sources(CoronaSystemAudio
    PUBLIC
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/systems/AudioSystem.h
)

target_compile_features(CoronaSystemAudio PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaSystemAudio)

target_link_libraries(CoronaSystemAudio
    PUBLIC
    corona::system::interface
    corona::core
)
