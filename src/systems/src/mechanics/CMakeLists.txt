add_library(corona_mechanics_system STATIC
    mechanics_system.cpp
)

add_library(corona::system::mechanics ALIAS corona_mechanics_system)

target_include_directories(corona_mechanics_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_mechanics_system PUBLIC
    corona::kernel
)
