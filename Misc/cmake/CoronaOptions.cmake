# CoronaOptions.cmake
# 项目级选项与高层功能开关集中定义文件
# 说明：
#   - 将所有可在命令行 -D 传入的布尔型/路径型/版本型控制项集中放置，便于统一管理与文档化。
#   - 若未来有更多特性开关（如：启用测试 / 启用安装 / 启用特定第三方库），建议也在此追加。

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# 是否构建为共享库 (ON -> 生成 .dll/.so；OFF -> 生成静态库)
option(BUILD_SHARED_LIBS "Build as shared libraries (default OFF for static)" OFF)
# 是否编译Corona编辑器 (ON -> 编译; OFF -> 不编译)
option(BUILD_CORONA_EDITOR "Build corona editor" OFF)

# 是否编译示例工程：
#   ON  -> 生成 Examples 目录下的演示可执行，便于调试 / 验证功能
#   OFF -> 跳过示例，减少依赖（如 GLFW）拉取与编译时间
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(CORONA_BUILD_EXAMPLES "Build example programs" ON)
else()
    option(CORONA_BUILD_EXAMPLES "Build example programs" OFF)
endif()

# Unified concise summary for key options (English output as per repository convention)
message(STATUS "[Options] BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}; BUILD_CORONA_EDITOR=${BUILD_CORONA_EDITOR}; CORONA_BUILD_EXAMPLES=${CORONA_BUILD_EXAMPLES}")
