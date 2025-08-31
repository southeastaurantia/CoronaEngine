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
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/include"
)

target_link_directories(CabbageFramework PUBLIC
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/lib/intel64/vc14"
)

target_link_libraries(CabbageFramework PUBLIC
        assimp
        Helicon
        EnTT::EnTT
        CabbageHardware

        $<$<CONFIG:Debug>:tbb12_debug>
        $<$<CONFIG:RelWithDebInfo>:tbb12_debug>
        $<$<CONFIG:Release>:tbb12>
        $<$<CONFIG:MinSizeRel>:tbb12>

        $<$<CONFIG:Debug>:tbbbind_2_5_debug>
        $<$<CONFIG:RelWithDebInfo>:tbbbind_2_5_debug>
        $<$<CONFIG:Release>:tbbbind_2_5>
        $<$<CONFIG:MinSizeRel>:tbbbind_2_5>

        $<$<CONFIG:Debug>:tbbmalloc_debug>
        $<$<CONFIG:RelWithDebInfo>:tbbmalloc_debug>
        $<$<CONFIG:Release>:tbbmalloc>
        $<$<CONFIG:MinSizeRel>:tbbmalloc>

        $<$<CONFIG:Debug>:tbbmalloc_proxy_debug>
        $<$<CONFIG:RelWithDebInfo>:tbbmalloc_proxy_debug>
        $<$<CONFIG:Release>:tbbmalloc_proxy>
        $<$<CONFIG:MinSizeRel>:tbbmalloc_proxy>
)

# 日志级别
# 0 - Debug
# 1 - Info
# 2 - Warn
# 3 - Error
target_compile_definitions(CabbageFramework PRIVATE LOG_LEVEL=1)

file(GLOB TBB_DLLS "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/redist/intel64/vc14/*.dll")
file(GLOB TBB_PDBS "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/redist/intel64/vc14/*.pdb")

if (TBB_DLLS AND TBB_PDBS)
    message(STATUS "Found TBB DLLs and PDBs")
    set_target_properties(CabbageFramework PROPERTIES
            INTERFACE_CABBAGE_RUNTIME_DEPS "${TBB_DLLS};${TBB_PDBS}"
    )
endif ()