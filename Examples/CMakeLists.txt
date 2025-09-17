# =====================================================================================
#  Example Sources
# =====================================================================================
file(GLOB_RECURSE EXAMPLES_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)
if(NOT EXAMPLES_SOURCE_FILES)
    message(WARNING "No example source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXAMPLES_SOURCE_FILES})

# =====================================================================================
#  Executable Target
# =====================================================================================
add_executable(CoronaEngineExamples ${EXAMPLES_SOURCE_FILES})

# VS 启动项目与调试工作目录
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CoronaEngineExamples)
set_target_properties(CoronaEngineExamples PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:CoronaEngineExamples>
)

target_link_libraries(CoronaEngineExamples PRIVATE
    glfw
    CoronaEngine
)

# =====================================================================================
#  Runtime Dependencies (Helicon + Corona)
# =====================================================================================
helicon_install_runtime_deps(CoronaEngineExamples)
corona_install_runtime_deps(CoronaEngineExamples)

# =====================================================================================
#  Asset Copy Step
# =====================================================================================
set(_EXAMPLES_ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
add_custom_command(
    TARGET CoronaEngineExamples POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copying assets (if exist) -> $<TARGET_FILE_DIR:CoronaEngineExamples>/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:CoronaEngineExamples>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_EXAMPLES_ASSETS_DIR}"
            $<TARGET_FILE_DIR:CoronaEngineExamples>/assets
    COMMENT "Copying assets directory"
    VERBATIM
)
