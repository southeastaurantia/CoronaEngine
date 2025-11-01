add_library(corona_rendering_system STATIC
    rendering_system.cpp
)

add_library(corona::system::rendering ALIAS corona_rendering_system)

target_include_directories(corona_rendering_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
)

target_link_libraries(corona_rendering_system PUBLIC
    corona::kernel
)