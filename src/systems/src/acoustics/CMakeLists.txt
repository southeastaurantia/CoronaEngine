add_library(corona_acoustics_system STATIC
    acoustics_system.cpp
)

add_library(corona::system::acoustics ALIAS corona_acoustics_system)

target_include_directories(corona_acoustics_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_acoustics_system PUBLIC
    corona::kernel
)
