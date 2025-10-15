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

target_compile_features(CoronaScriptPython PUBLIC cxx_std_20)

target_include_directories(CoronaScriptPython
    PUBLIC
    ${_CORONA_SCRIPT_PY_MODULE_DIR}/public
    ${Python3_INCLUDE_DIRS}
)

target_link_directories(CoronaScriptPython
    PUBLIC
    ${Python3_LIBRARY_DIRS}
)

target_link_libraries(CoronaScriptPython
    PUBLIC
    corona::core
    corona::utility::logger

    python313
)
