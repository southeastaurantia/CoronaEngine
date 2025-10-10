# 先构建 utility 模块
add_subdirectory(utility)

# 收集所有 C/C++ 源与头文件，CONFIGURE_DEPENDS 确保新增文件被自动发现
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

if(NOT SOURCE_FILES)
        message(WARNING "[src] 未在 ${CMAKE_CURRENT_SOURCE_DIR} 中找到任何源文件，请确认路径或文件扩展名。")
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

# 主引擎以静态库形式提供，示例应用直接链接
add_library(CoronaEngine STATIC ${SOURCE_FILES})

target_compile_features(CoronaEngine PUBLIC cxx_std_20)

target_include_directories(CoronaEngine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" # 允许头按相对路径引用
        "${CMAKE_CURRENT_SOURCE_DIR}/include" # 公共 API 头文件目录
        "${CMAKE_CURRENT_SOURCE_DIR}/common/include" # 全局头文件目录
        "${stb_SOURCE_DIR}" # stb 单头库
        ${Python3_INCLUDE_DIRS}
)

target_link_directories(CoronaEngine PUBLIC
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
)

# 将运行时依赖写入目标属性，供示例和应用复制
corona_configure_runtime_deps(CoronaEngine)

if(BUILD_CORONA_EDITOR)
        message(STATUS "[Editor] Configure resources for CoronaEngine")
        corona_configure_corona_editor(CoronaEngine)
endif()
