# =====================================================================================
#  Source Collection
# =====================================================================================
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)
if(NOT SOURCE_FILES)
        message(WARNING "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

# =====================================================================================
#  Library Target
# =====================================================================================
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
        # TBB core
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbb12_debug,tbb12>
        # tbbbind
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbbind_2_5_debug,tbbbind_2_5>
        # tbbmalloc
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbmalloc_debug,tbbmalloc>
        # tbbmalloc_proxy
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbmalloc_proxy_debug,tbbmalloc_proxy>
)

# =====================================================================================
#  Runtime Dependency Collection (DLL / PDB)
# =====================================================================================
file(GLOB TBB_DLLS  "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/redist/intel64/vc14/*.dll")
file(GLOB TBB_PDBS  "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/redist/intel64/vc14/*.pdb")
file(GLOB PYTHON_DLLS "${Python3_RUNTIME_LIBRARY_DIRS}/*.dll")
file(GLOB PYTHON_PDBS "${Python3_RUNTIME_LIBRARY_DIRS}/*.pdb")

if(TBB_DLLS AND TBB_PDBS AND PYTHON_DLLS AND PYTHON_PDBS)
        message(STATUS "Found TBB + Python runtime binaries (DLL/PDB)")
        set_target_properties(CoronaEngine PROPERTIES
                        INTERFACE_CORONA_RUNTIME_DEPS "${TBB_DLLS};${TBB_PDBS};${PYTHON_DLLS};${PYTHON_PDBS}"
        )
else()
        if(NOT TBB_DLLS)
                message(WARNING "TBB DLLs not found for CoronaEngine runtime package")
        endif()
        if(NOT PYTHON_DLLS)
                message(WARNING "Python DLLs not found in ${Python3_RUNTIME_LIBRARY_DIRS}")
        endif()
endif()
