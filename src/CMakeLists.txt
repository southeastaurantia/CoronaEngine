include_guard(GLOBAL)

set(_CORONA_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(utils)
add_subdirectory(utility)
add_subdirectory(thread)
add_subdirectory(resource)
add_subdirectory(systems)
add_subdirectory(core)
add_subdirectory(script)

add_library(CoronaEngine STATIC)
add_library(corona::engine ALIAS CoronaEngine)

set_property(TARGET CoronaEngine PROPERTY OUTPUT_NAME "CoronaEngine")

# Link all discovered module targets (public to propagate includes/compile features)

foreach(_module_target IN ITEMS
        CoronaCore
        CoronaResource
        CoronaScriptPython
        CoronaSystemAnimation
        CoronaSystemAudio
        CoronaSystemDisplay
        CoronaSystemRendering
        CoronaThread
        CoronaUtilityLogger
        CoronaUtilityResource
        CoronaUtils)
    if(TARGET ${_module_target})
        target_link_libraries(CoronaEngine PUBLIC ${_module_target})
    endif()
endforeach()

target_compile_features(CoronaEngine PUBLIC cxx_std_20)
