
cmake_minimum_required(VERSION 4.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =====================================================================================
# 工程元数据 (项目名称 / 版本 / 主页 / 使用语言)
# - VERSION: 统一用于后续安装导出、包配置以及可能的语义版本判断
# - LANGUAGES: 仅启用 C / CXX，避免不必要的语言检查加快配置速度
# =====================================================================================
project(CoronaEngine VERSION 0.5.0 HOMEPAGE_URL "https://github.com/CoronaEngine/CoronaEngine" LANGUAGES C CXX)

# =====================================================================================
# 模块化引入 (保持根 CMakeLists 精简，只承担“编排”职责)
# 每个模块职责：
# CoronaOptions.cmake        -> 项目级选项/开关定义
# CoronaPython.cmake         -> Python 解释器发现 + 依赖检测
# CoronaRuntimeDeps.cmake    -> 运行时依赖(TBB/Python dll/pdb)收集与复制函数
# CoronaCompileConfig.cmake  -> 编译宏 / MSVC 运行库策略
# CoronaThirdParty.cmake     -> 第三方库 FetchContent 声明与拉取
# 如需新增模块（安装/测试等）建议继续追加 include 行保持结构清晰。
# =====================================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaOptions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaPython.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaRuntimeDeps.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaCompileConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaThirdParty.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/BuildCabbageEditor.cmake)

add_subdirectory(Src) # 核心引擎库

if(CORONA_BUILD_EXAMPLES)
    add_subdirectory(Examples) # 可选示例程序 (由 CORONA_BUILD_EXAMPLES 控制)
endif()

# =====================================================================================
# 配置结果汇总 (方便在 CI / 本地快速确认关键选项生效情况)
# =====================================================================================
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