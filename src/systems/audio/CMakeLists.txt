include_guard(GLOBAL)

set(_CORONA_SYSTEM_AUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(SystemAudio ${_CORONA_SYSTEM_AUDIO_DIR})

add_library(CoronaSystemAudio STATIC)
add_library(corona::system::audio ALIAS CoronaSystemAudio)

if(CORONA_SYSTEMAUDIO_PRIVATE_SOURCES)
    target_sources(CoronaSystemAudio PRIVATE ${CORONA_SYSTEMAUDIO_PRIVATE_SOURCES})
endif()

if(CORONA_SYSTEMAUDIO_PUBLIC_HEADERS)
    target_sources(CoronaSystemAudio PUBLIC ${CORONA_SYSTEMAUDIO_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaSystemAudio PUBLIC cxx_std_20)

target_include_directories(CoronaSystemAudio
    PUBLIC
        ${_CORONA_SYSTEM_AUDIO_DIR}/public
)

target_link_libraries(CoronaSystemAudio
    PUBLIC
    corona::system::interface
    corona::core
)
