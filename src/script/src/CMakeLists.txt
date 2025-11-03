# CoronaEngine - Script (Python) binding module
# Mirrors the simple system CMake patterns (static lib + alias + includes + links)

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
if(DEFINED Python_INCLUDE_DIRS)
    target_include_directories(CoronaScriptPython PUBLIC ${Python_INCLUDE_DIRS})
elseif(DEFINED Python3_INCLUDE_DIRS)
    target_include_directories(CoronaScriptPython PUBLIC ${Python3_INCLUDE_DIRS})
endif()

# nanobind headers (header-only). FetchContent sets nanobind_SOURCE_DIR.
if(DEFINED nanobind_SOURCE_DIR)
    target_include_directories(CoronaScriptPython PUBLIC ${nanobind_SOURCE_DIR}/include)
endif()

# Link dependencies
# - Keep consistent with systems (link corona::kernel)
# - Prefer imported Python target if provided by FindPython
set(_corona_script_python_link_libs corona::kernel)
if(TARGET Python::Python)
    list(APPEND _corona_script_python_link_libs Python::Python)
elseif(TARGET Python3::Python)
    list(APPEND _corona_script_python_link_libs Python3::Python)
endif()

target_link_libraries(CoronaScriptPython PUBLIC
        ${_corona_script_python_link_libs}
        ktm
        EnTT::EnTT
)

# C++ standard to align with project settings
set_property(TARGET CoronaScriptPython PROPERTY CXX_STANDARD 20)
set_property(TARGET CoronaScriptPython PROPERTY CXX_STANDARD_REQUIRED ON)

message(STATUS "[CoronaScriptPython] Script binding library configured")
