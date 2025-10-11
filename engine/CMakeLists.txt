set(CORONA_ENGINE_RUNTIME_TARGET Corona_runtime)

add_executable(${CORONA_ENGINE_RUNTIME_TARGET}
    main.cpp
)

target_compile_features(${CORONA_ENGINE_RUNTIME_TARGET} PRIVATE cxx_std_20)
set_target_properties(${CORONA_ENGINE_RUNTIME_TARGET} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>
)

target_link_libraries(${CORONA_ENGINE_RUNTIME_TARGET} PRIVATE CoronaEngine)

if(COMMAND helicon_install_runtime_deps)
    helicon_install_runtime_deps(${CORONA_ENGINE_RUNTIME_TARGET})
else()
    message(STATUS "[Corona:EngineRuntime] helicon_install_runtime_deps not available; skip")
endif()

corona_install_runtime_deps(${CORONA_ENGINE_RUNTIME_TARGET})

if(BUILD_CORONA_EDITOR)
    corona_install_corona_editor(${CORONA_ENGINE_RUNTIME_TARGET} CoronaEngine)
endif()
