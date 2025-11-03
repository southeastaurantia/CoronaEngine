add_library(corona_display_system STATIC
    display_system.cpp
)

add_library(corona::system::display ALIAS corona_display_system)

target_include_directories(corona_display_system PUBLIC
    ${PROJECT_SOURCE_DIR}/src/systems/include
    ${PROJECT_SOURCE_DIR}/src/include
)

target_link_libraries(corona_display_system PUBLIC
    corona::kernel
)