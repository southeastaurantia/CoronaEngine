add_library(corona_optics_system STATIC
    optics_system.cpp
)

add_library(corona::system::optics ALIAS corona_optics_system)

target_include_directories(corona_optics_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_optics_system PUBLIC
    corona::kernel
)
