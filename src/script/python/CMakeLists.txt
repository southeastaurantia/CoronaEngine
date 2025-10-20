include_guard(GLOBAL)

set(_CORONA_SCRIPT_PY_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(ScriptPython ${_CORONA_SCRIPT_PY_MODULE_DIR})

add_library(CoronaScriptPython STATIC)
add_library(corona::script::python ALIAS CoronaScriptPython)

if(CORONA_SCRIPTPYTHON_PRIVATE_SOURCES)
    target_sources(CoronaScriptPython PRIVATE ${CORONA_SCRIPTPYTHON_PRIVATE_SOURCES})
endif()

if(CORONA_SCRIPTPYTHON_PUBLIC_HEADERS)
    target_sources(CoronaScriptPython PUBLIC ${CORONA_SCRIPTPYTHON_PUBLIC_HEADERS})
endif()

target_sources(CoronaScriptPython
    PUBLIC
        ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/script/EngineScripts.h
        ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/script/PythonAPI.h
        ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/script/PythonBridge.h
        ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/script/PythonHotfix.h
)

target_compile_features(CoronaScriptPython PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaScriptPython)

target_include_directories(CoronaScriptPython
    PUBLIC
        ${Python3_INCLUDE_DIRS}
)

# Add nanobind include directory if available
if(DEFINED nanobind_SOURCE_DIR)
    target_include_directories(CoronaScriptPython PUBLIC ${nanobind_SOURCE_DIR}/include)
    message(STATUS "TEST NANOBIND-----------------------------")
endif()

# Ensure Python::Module/Interpreter targets exist for nanobind
if (NOT TARGET Python::Module OR NOT TARGET Python::Interpreter)
    set(Python_FIND_FRAMEWORK LAST)
    if (CMAKE_VERSION VERSION_LESS 4.0)
        set(_NB_DEV_MODULE Development)
    else()
        set(_NB_DEV_MODULE Development.Module)
    endif()
    find_package(Python 3.13 REQUIRED COMPONENTS Interpreter ${_NB_DEV_MODULE})
endif()

# Build/link nanobind core static library if helper is available
if (COMMAND nanobind_build_library)
    nanobind_build_library(nanobind-static AS_SYSINCLUDE)
    target_link_libraries(CoronaScriptPython PUBLIC nanobind-static)
endif()

# Python lib dirs

target_link_directories(CoronaScriptPython
    PUBLIC
    ${Python3_LIBRARY_DIRS}
)

# Link to engine, logger, and Python

target_link_libraries(CoronaScriptPython
    PUBLIC
    corona::core
    Corona::Logger

    python313
)
