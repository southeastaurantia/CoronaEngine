# =====================================================================================
# 源码收集 (Source Collection)
# 说明：递归收集当前目录下所有常见 C/C++ 源/头文件；若结果为空给出警告，方便定位路径或过滤配置错误。
# 使用 CONFIGURE_DEPENDS 使新增文件在再次 configure 时被自动感知。
# =====================================================================================
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

# =====================================================================================
# 库目标定义 (Library Target)
# 说明：目前构建为 STATIC 静态库，便于示例/应用直接链接；后续若需安装共享库可切换为 SHARED 并调整导出逻辑。
# =====================================================================================
add_library(CoronaEngine STATIC ${SOURCE_FILES})

target_compile_options(CoronaEngine PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_include_directories(CoronaEngine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" # 本工程源目录（允许头按相对路径引用）
        "${stb_SOURCE_DIR}" # stb 单头库目录
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/include" # TBB 头文件
        "${PROJECT_SOURCE_DIR}/Env/spdlog-1.15.3/include" # spdlog 头文件（Header-only 可直接用）
        ${Python3_INCLUDE_DIRS} # Python C API 头路径（来自解释器发现）
)

target_link_directories(CoronaEngine PUBLIC
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/lib/intel64/vc14" # TBB 静态/动态库目录
        ${Python3_LIBRARY_DIRS} # Python 运行库目录
)

target_link_libraries(CoronaEngine PUBLIC
        assimp # 模型/场景导入
        Helicon # (外部子模块/功能库，若后续移除请同步删引用)
        EnTT::EnTT # ECS 框架
        CabbageHardware # 硬件/平台适配层
        python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR} # Python 主/次版本动态库

        # 以下使用生成器表达式按配置选择 Debug / Release 变体，减少手工条件判断
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbb12_debug,tbb12> # TBB core
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbbind_2_5_debug,tbbbind_2_5> # tbbbind
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbmalloc_debug,tbbmalloc> # 内存分配器
        $<IF:$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>,tbbmalloc_proxy_debug,tbbmalloc_proxy> # 分配器代理
)

# =====================================================================================
# 运行时依赖收集 (Runtime Dependency Collection)
# 说明：调用统一函数收集 TBB / Python DLL + PDB 列表并写入 CoronaEngine 的接口属性，供示例或其它可执行复制。
# =====================================================================================
corona_configure_runtime_deps(CoronaEngine)

if(BUILD_CORONA_EDITOR)
        message(STATUS "[Editor] Configure resources for CoronaEngine")
        corona_configure_corona_editor(CoronaEngine)
endif()
