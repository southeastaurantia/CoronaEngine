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

target_link_directories(CoronaScriptPython
    PUBLIC
    ${Python3_LIBRARY_DIRS}
)

target_link_libraries(CoronaScriptPython
    PUBLIC
    corona::core
    Corona::Logger

    python313
)
