# ==============================================================================
# corona_options.cmake
# ==============================================================================
# 功能：项目级选项与高层功能开关集中定义
#
# 说明：
#   - 将所有可在命令行 -D 传入的布尔型/路径型/版本型控制项集中放置
#   - 便于统一管理与文档化
#   - 未来有更多特性开关（如启用测试/启用安装/启用特定第三方库）也在此追加
# ==============================================================================

include_guard(GLOBAL)

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# ------------------------------------------------------------------------------
# 构建选项
# ------------------------------------------------------------------------------

# 是否构建为共享库
option(BUILD_SHARED_LIBS 
    "Build as shared libraries (default OFF for static)" 
    OFF)

# 是否编译 Corona 编辑器
option(BUILD_CORONA_EDITOR 
    "Build Corona editor" 
    OFF)

# 是否编译 Corona 运行时主程序
option(BUILD_CORONA_RUNTIME 
    "Build Corona runtime" 
    ON)

# 是否编译示例工程
option(CORONA_BUILD_EXAMPLES 
    "Build example programs" 
    ${PROJECT_IS_TOP_LEVEL})

# ------------------------------------------------------------------------------
# 配置摘要
# ------------------------------------------------------------------------------
message(STATUS "[Options] BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}")
message(STATUS "[Options] BUILD_CORONA_EDITOR=${BUILD_CORONA_EDITOR}")
message(STATUS "[Options] BUILD_CORONA_RUNTIME=${BUILD_CORONA_RUNTIME}")
message(STATUS "[Options] CORONA_BUILD_EXAMPLES=${CORONA_BUILD_EXAMPLES}")
