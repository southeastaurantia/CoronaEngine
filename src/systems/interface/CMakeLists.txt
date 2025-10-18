include_guard(GLOBAL)

add_library(CoronaSystemInterface INTERFACE)
add_library(corona::system::interface ALIAS CoronaSystemInterface)

target_link_libraries(CoronaSystemInterface
    INTERFACE
        corona::interfaces
        Corona::Logger
)