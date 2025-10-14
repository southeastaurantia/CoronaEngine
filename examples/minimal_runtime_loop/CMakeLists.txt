# Minimal example using Engine + RuntimeLoop

# Use the helper to define the example target
corona_add_example(
    NAME minimal_runtime_loop
    SOURCES main.cpp
    COPY_ASSETS
)

# Allow this example to include the RuntimeLoop header from the engine module
# Use project root so <engine/RuntimeLoop.h> resolves
target_include_directories(minimal_runtime_loop PRIVATE ${PROJECT_SOURCE_DIR})

# Pull in the RuntimeLoop implementation and required dependencies
target_sources(minimal_runtime_loop PRIVATE ${PROJECT_SOURCE_DIR}/engine/RuntimeLoop.cpp)

target_link_libraries(minimal_runtime_loop PRIVATE EnTT::EnTT)
