add_library(corona_audio_system STATIC
    audio_system.cpp
)

add_library(corona::system::audio ALIAS corona_audio_system)

target_include_directories(corona_audio_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_audio_system PUBLIC
    corona::kernel
)