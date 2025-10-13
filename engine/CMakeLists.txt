# ==============================================================================
# CoronaEngine - 主程序可执行文件构建配置
# ==============================================================================
# 说明：
# - 创建 Corona_runtime 可执行文件（引擎主程序）
# - 安装所有运行时依赖（Helicon/Python DLLs/编辑器资源/Assets）
# ==============================================================================

set(CORONA_ENGINE_RUNTIME_TARGET Corona_runtime)

# ------------------------------------------------------------------------------
# 创建可执行目标
# ------------------------------------------------------------------------------
add_executable(${CORONA_ENGINE_RUNTIME_TARGET}
    main.cpp
)

target_compile_features(${CORONA_ENGINE_RUNTIME_TARGET} PRIVATE cxx_std_20)
set_target_properties(${CORONA_ENGINE_RUNTIME_TARGET} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>
)

target_link_libraries(${CORONA_ENGINE_RUNTIME_TARGET} PRIVATE
    CoronaEngine
    EnTT::EnTT
)

# ------------------------------------------------------------------------------
# 安装运行时依赖
# ------------------------------------------------------------------------------

# 安装 Helicon/Vulkan 运行时依赖 (DLLs)
if(COMMAND helicon_install_runtime_deps)
    helicon_install_runtime_deps(${CORONA_ENGINE_RUNTIME_TARGET})
else()
    message(STATUS "[Corona:EngineRuntime] helicon_install_runtime_deps not available; skip")
endif()

# 安装 Corona 运行时依赖 (Python DLLs/PDBs 等)
corona_install_runtime_deps(${CORONA_ENGINE_RUNTIME_TARGET})

# 安装编辑器资源 (Backend/Frontend)
if(BUILD_CORONA_EDITOR)
    corona_install_corona_editor(${CORONA_ENGINE_RUNTIME_TARGET} CoronaEngine)
endif()

# 拷贝 assets 资源文件到运行时目录
add_custom_command(TARGET ${CORONA_ENGINE_RUNTIME_TARGET} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "[Corona_runtime] Copy assets to $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" $<TARGET_FILE_DIR:${CORONA_ENGINE_RUNTIME_TARGET}>/assets
    COMMENT "[Corona_runtime] Installing assets directory"
    VERBATIM
)

message(STATUS "[Corona:EngineRuntime] ${CORONA_ENGINE_RUNTIME_TARGET} configured with full runtime dependencies")
