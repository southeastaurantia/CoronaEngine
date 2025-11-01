add_library(corona_animation_system STATIC
    animation_system.cpp
)

add_library(corona::system::animation ALIAS corona_animation_system)

target_include_directories(corona_animation_system PRIVATE
    ${PROJECT_SOURCE_DIR}/src/systems/include
)

target_link_libraries(corona_animation_system PRIVATE
    corona::kernel
)