file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})
add_library(CoronaEngine STATIC ${SOURCE_FILES})

target_compile_options(CoronaEngine PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8> # 使用 UTF-8 编码
)

target_include_directories(CoronaEngine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${stb_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/include"
        "${PROJECT_SOURCE_DIR}/Env/spdlog-1.15.3/include"
        ${Python3_INCLUDE_DIRS}
)

target_link_directories(CoronaEngine PUBLIC
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/lib/intel64/vc14"
        ${Python3_LIBRARY_DIRS}
)

target_link_libraries(CoronaEngine PUBLIC
        assimp
        Helicon
        EnTT::EnTT
        CabbageHardware
        python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}

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

file(GLOB TBB_DLLS "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/redist/intel64/vc14/*.dll")
file(GLOB TBB_PDBS "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/redist/intel64/vc14/*.pdb")
file(GLOB PYTHON_DLLS "${Python3_RUNTIME_LIBRARY_DIRS}/*.dll")
file(GLOB PYTHON_PDBS "${Python3_RUNTIME_LIBRARY_DIRS}/*.pdb")

if(TBB_DLLS AND TBB_PDBS AND PYTHON_DLLS AND PYTHON_PDBS)
        message(STATUS "Found TBB DLLs and PDBs and PYTHON_DLLS and PDBS")
        set_target_properties(CoronaEngine PROPERTIES
                INTERFACE_CABBAGE_RUNTIME_DEPS "${TBB_DLLS};${TBB_PDBS};${PYTHON_DLLS};${PYTHON_PDBS}"
        )
endif()