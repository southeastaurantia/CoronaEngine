file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
add_library(CabbageFramework STATIC ${SOURCE_FILES})

target_include_directories(CabbageFramework PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${stb_SOURCE_DIR}"
)

target_link_libraries(CabbageFramework PUBLIC
    assimp
    Helicon
    EnTT::EnTT
    CabbageHardware
    marl
)

# 日志级别
# 0 - Debug
# 1 - Info
# 2 - Warn
# 3 - Error
target_compile_definitions(CabbageFramework PRIVATE LOG_LEVEL=1)