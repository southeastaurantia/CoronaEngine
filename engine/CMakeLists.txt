# ============================================================================== 
# CoronaEngine - Runtime Executable Build Configuration
#
# Responsibilities:
#   - Build the `Corona_runtime` executable that hosts the default engine loop.
#   - Stage runtime dependencies, editor resources, and shared assets after build.
# ============================================================================== 

set(CORONA_ENGINE_RUNTIME_TARGET Corona_runtime)

# ------------------------------------------------------------------------------
# Executable Target
# ------------------------------------------------------------------------------
add_executable(
    ${CORONA_ENGINE_RUNTIME_TARGET}
    RuntimeLoop.cpp
    main.cpp
)

target_compile_features(${CORONA_ENGINE_RUNTIME_TARGET} PRIVATE cxx_std_20)

set_target_properties(
    ${CORONA_ENGINE_RUNTIME_TARGET}
    PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>
)

target_link_libraries(
    ${CORONA_ENGINE_RUNTIME_TARGET}
    PRIVATE
        CoronaEngine
        EnTT::EnTT
)

# ------------------------------------------------------------------------------
# Runtime Dependencies and Assets
# ------------------------------------------------------------------------------
if(COMMAND helicon_install_runtime_deps)
    helicon_install_runtime_deps(${CORONA_ENGINE_RUNTIME_TARGET})
else()
    message(STATUS "[Corona:EngineRuntime] helicon_install_runtime_deps not available; skipping Helicon runtime dependencies")
endif()

corona_install_runtime_deps(${CORONA_ENGINE_RUNTIME_TARGET})

if(BUILD_CORONA_EDITOR)
    corona_install_corona_editor(${CORONA_ENGINE_RUNTIME_TARGET} CoronaEngine)
endif()

add_custom_command(
    TARGET      ${CORONA_ENGINE_RUNTIME_TARGET}
    POST_BUILD
    COMMAND     ${CMAKE_COMMAND} -E echo "[Corona_runtime] Copy assets to $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>/assets"
    COMMAND     ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>/assets"
    COMMAND     ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>/assets"
    COMMENT     "[Corona_runtime] Installing assets directory"
    VERBATIM
)

message(STATUS "[Corona:EngineRuntime] ${CORONA_ENGINE_RUNTIME_TARGET} configured with full runtime dependencies")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR})
