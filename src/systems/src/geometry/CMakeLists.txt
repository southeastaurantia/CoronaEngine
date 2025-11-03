add_library(corona_geometry_system STATIC
    geometry_system.cpp
)

add_library(corona::system::geometry ALIAS corona_geometry_system)

target_include_directories(corona_geometry_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_geometry_system PUBLIC
    corona::kernel
)
