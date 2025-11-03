add_library(corona_animation_system STATIC
    animation_system.cpp
)

add_library(corona::system::animation ALIAS corona_animation_system)

target_include_directories(corona_animation_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_animation_system PUBLIC
    corona::kernel
)