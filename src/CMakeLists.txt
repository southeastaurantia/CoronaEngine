# ============================================================================== 
# CoronaEngine - Source Modules Aggregation
#
# Responsibilities:
#   - Add all engine module subdirectories.
#   - Create the unified static library target `CoronaEngine`.
#   - Propagate public include directories and compiler features to dependents.
# ============================================================================== 

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# Module Subdirectories
# ------------------------------------------------------------------------------
add_subdirectory(interfaces)     # Public interfaces and headers exposed externally
add_subdirectory(utils)          # Shared utilities
add_subdirectory(thread)         # Threading primitives and concurrency helpers
add_subdirectory(systems)        # Engine systems (animation/audio/display/rendering)
add_subdirectory(core)           # Engine core (Engine singleton and services)
add_subdirectory(script)         # Scripting integration (Python bridge)

# ------------------------------------------------------------------------------
# CoronaEngine Static Library
# ------------------------------------------------------------------------------
add_library(CoronaEngine STATIC)
add_library(corona::engine ALIAS CoronaEngine)

set_property(TARGET CoronaEngine PROPERTY OUTPUT_NAME "CoronaEngine")

set(_corona_modules
    CoronaCore
    CoronaResource
    CoronaScriptPython
    CoronaSystemAnimation
    CoronaSystemAudio
    CoronaSystemDisplay
    CoronaSystemRendering
    CoronaThread
    CoronaLogger
    CoronaUtilityResource
    CoronaUtils
)

foreach(_module_target IN LISTS _corona_modules)
    if(TARGET ${_module_target})
        target_link_libraries(CoronaEngine PUBLIC ${_module_target})
    endif()
endforeach()

target_compile_features(CoronaEngine PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaEngine)

if(COMMAND corona_configure_runtime_deps)
    corona_configure_runtime_deps(CoronaEngine)
endif()

if(BUILD_CORONA_EDITOR)
    corona_configure_corona_editor(CoronaEngine)
endif()
