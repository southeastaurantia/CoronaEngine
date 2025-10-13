cmake_minimum_required(VERSION 4.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/misc/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 基础项目信息，仅启用 C/C++ 并保留统一版本号
project(CoronaEngine VERSION 0.5.0 HOMEPAGE_URL "https://github.com/CoronaEngine/CoronaEngine" LANGUAGES C CXX)

# 模块化拆分：保持根脚本专注于 orchestrate
include(corona_collect_module)
include(corona_options) # 全局开关
include(corona_python) # Python 探测与依赖校验
include(corona_runtime_deps) # 运行时 DLL/PDB 收集
include(corona_compile_config) # 编译宏与 MSVC 运行库
include(corona_third_party) # FetchContent 第三方依赖
include(corona_editor) # 编辑器资源安装

# 库与主引擎
add_subdirectory(src)

if(BUILD_CORONA_RUNTIME)
    add_subdirectory(engine)
endif()

# 示例工程按需开启
if(CORONA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 配置摘要，方便在 CI / 本地快速核对
message(STATUS "================ CoronaEngine Configuration Summary ==================")
message(STATUS "[Config] Python executable : ${Python3_EXECUTABLE}")

if(Python3_VERSION)
    message(STATUS "[Config] Python version    : ${Python3_VERSION}")
endif()

message(STATUS "[Config] Build examples    : ${CORONA_BUILD_EXAMPLES}")
message(STATUS "[Config] Build editor      : ${BUILD_CORONA_EDITOR}")
message(STATUS "[Config] Check py deps     : ${CORONA_CHECK_PY_DEPS}")
message(STATUS "[Config] Auto install deps : ${CORONA_AUTO_INSTALL_PY_DEPS}")
message(STATUS "======================================================================")