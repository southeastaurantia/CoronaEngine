# CoronaEngine - Script (Python) binding module
# Mirrors the simple system CMake patterns (static lib + alias + includes + links)

# Build nanobind static library
# nanobind is NOT header-only, it needs to be compiled
if(DEFINED nanobind_SOURCE_DIR)
    # Use nanobind's build function to create the library target
    include(${nanobind_SOURCE_DIR}/cmake/nanobind-config.cmake)
    nanobind_build_library(nanobind-static)
else()
    message(FATAL_ERROR "[CMake] nanobind source directory not found; cannot continue")
endif()

# Sources
add_library(CoronaScriptPython STATIC
    corona_engine_api.cpp
    corona_engine_module.cpp
    engine_bindings.cpp
    python_api.cpp
    python_bridge.cpp
    python_hotfix.cpp
)

# Public alias used by core engine
add_library(corona::script::python ALIAS CoronaScriptPython)

# Public include directories (match systems style and expose script headers)
target_include_directories(CoronaScriptPython PUBLIC
    ${PROJECT_SOURCE_DIR}/src/script/include
    ${PROJECT_SOURCE_DIR}/src/include
    $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
)

# Python include dirs (public because PythonAPI.h includes <Python.h>)
target_include_directories(CoronaScriptPython PUBLIC
    ${Python_INCLUDE_DIRS}
    ${nanobind_SOURCE_DIR}/include
)

target_link_directories(CoronaScriptPython PUBLIC
    ${Python_LIBRARY_DIRS}
)

target_link_libraries(CoronaScriptPython PUBLIC
    ktm
    corona::kernel
    Python::Python
    EnTT::EnTT
    nanobind-static
    CoronaResource::Resource
    CabbageHardware
)

# C++ standard to align with project settings
set_property(TARGET CoronaScriptPython PROPERTY CXX_STANDARD 20)
set_property(TARGET CoronaScriptPython PROPERTY CXX_STANDARD_REQUIRED ON)

message(STATUS "[CoronaScriptPython] Script binding library configured")
