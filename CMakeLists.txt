# ==============================================================================
# CoronaEngine - 主 CMake 配置文件
#
# 功能:
#   - 协调整个项目的构建流程，采用模块化设计。
#   - 引入各个功能模块的 CMake 脚本（options/python/third-party/compile-config 等）。
#   - 控制子目录的添加顺序（src -> engine -> examples）。
# ==============================================================================

cmake_minimum_required(VERSION 4.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/misc/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------------------------
# 项目声明
# ------------------------------------------------------------------------------
project(CoronaEngine
    VERSION      0.5.0
    HOMEPAGE_URL "https://github.com/CoronaEngine/CoronaEngine"
    LANGUAGES    C CXX
)

# ------------------------------------------------------------------------------
# 引入模块化 CMake 脚本
# ------------------------------------------------------------------------------
include(corona_collect_module)      # 源文件收集工具函数
include(corona_options)             # 全局构建选项 (BUILD_CORONA_EDITOR/EXAMPLES 等)
include(corona_python)              # Python 环境探测与依赖校验
include(corona_runtime_deps)        # 运行时依赖收集与拷贝 (DLL/PDB)
include(corona_compile_config)      # 编译宏定义与编译器配置
include(corona_third_party)         # 第三方依赖 (FetchContent)
include(corona_editor)              # 编辑器资源安装

# ------------------------------------------------------------------------------
# 添加子目录
# ------------------------------------------------------------------------------
# 核心引擎库 (静态库 CoronaEngine)
add_subdirectory(src)

# 引擎主程序 (Corona_runtime 可执行文件)
if(BUILD_CORONA_RUNTIME)
    add_subdirectory(engine)
endif()

# 示例程序 (按需开启)
if(CORONA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ------------------------------------------------------------------------------
# 配置摘要
# ------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "  CoronaEngine Configuration Summary")
message(STATUS "================================================================================")
message(STATUS "  Python executable     : ${Python3_EXECUTABLE}")

if(Python3_VERSION)
    message(STATUS "  Python version        : ${Python3_VERSION}")
endif()

message(STATUS "  Build runtime         : ${BUILD_CORONA_RUNTIME}")
message(STATUS "  Build examples        : ${CORONA_BUILD_EXAMPLES}")
message(STATUS "  Build editor          : ${BUILD_CORONA_EDITOR}")
message(STATUS "  Check Python deps     : ${CORONA_CHECK_PY_DEPS}")
message(STATUS "  Auto install deps     : ${CORONA_AUTO_INSTALL_PY_DEPS}")
message(STATUS "================================================================================")
message(STATUS "")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR})