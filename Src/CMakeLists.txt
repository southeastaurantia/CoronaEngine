# 收集所有 C/C++ 源与头文件，CONFIGURE_DEPENDS 确保新增文件被自动发现
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

if(NOT SOURCE_FILES)
        message(WARNING "[Src] 未在 ${CMAKE_CURRENT_SOURCE_DIR} 中找到任何源文件，请确认路径或文件扩展名。")
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

# 主引擎以静态库形式提供，示例应用直接链接
add_library(CoronaEngine STATIC ${SOURCE_FILES})

target_compile_features(CoronaEngine PUBLIC cxx_std_20)

target_include_directories(CoronaEngine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" # 允许头按相对路径引用
        "${PROJECT_SOURCE_DIR}/Common/include" # 全局头文件目录
        "${stb_SOURCE_DIR}" # stb 单头库
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/include"
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
        CoronaLogger
        CoronaResources
        python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}

        # 生成器表达式按配置切换 Debug / Release 依赖
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbb12_debug,tbb12>
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbbind_2_5_debug,tbbbind_2_5>
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbmalloc_debug,tbbmalloc>
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbmalloc_proxy_debug,tbbmalloc_proxy>
)

# 将运行时依赖写入目标属性，供示例和应用复制
corona_configure_runtime_deps(CoronaEngine)

if(BUILD_CORONA_EDITOR)
        message(STATUS "[Editor] Configure resources for CoronaEngine")
        corona_configure_corona_editor(CoronaEngine)
endif()
