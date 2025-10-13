# ==============================================================================
# corona_third_party.cmake
# ==============================================================================
# 功能：外部第三方依赖声明与获取（基于 FetchContent）
#
# 说明：
#   - 集中列出引擎/示例需要的源码级依赖
#   - 使用 CMake 内置 FetchContent 在配置阶段按需拉取
#
# 优势：
#   - 避免手工预下载，并行发起拉取
#   - 统一管理版本，支持后续切换为本地 override
#
# 提示：
#   - 若需锁定稳定性，可将 GIT_TAG 从 main/master 改为特定提交哈希或发布版本号
# ==============================================================================

include_guard(GLOBAL)

include(FetchContent)

# ------------------------------------------------------------------------------
# 核心依赖声明
# ------------------------------------------------------------------------------

FetchContent_Declare(
    CabbageHardware
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageHardware.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    CabbageConcurrent
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageConcurrent.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(
    CoronaResource
    GIT_REPOSITORY https://github.com/CoronaEngine/CoronaResource.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# ------------------------------------------------------------------------------
# 拉取并添加核心依赖
# ------------------------------------------------------------------------------
FetchContent_MakeAvailable(assimp stb entt CabbageHardware CabbageConcurrent CoronaResource)
message(STATUS "[3rdparty] Core dependencies: assimp, stb, entt, CabbageHardware, CabbageConcurrent, CoronaResource")

# ------------------------------------------------------------------------------
# 示例程序依赖（按需启用）
# ------------------------------------------------------------------------------

if(CORONA_BUILD_EXAMPLES)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(glfw)
    message(STATUS "[3rdparty] Examples dependencies: glfw")
endif()
