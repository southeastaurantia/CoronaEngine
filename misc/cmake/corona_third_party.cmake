# CoronaThirdParty.cmake
# 外部第三方依赖声明与获取（基于 FetchContent）
# 说明：集中列出引擎/示例需要的源码级依赖，使用 CMake 内置 FetchContent 在配置阶段按需拉取。
# 好处：避免手工预下载并行发起拉取；统一管理版本；支持后续切换为本地 override。
# 提示：若需锁定稳定性，可将 GIT_TAG 从 master/main 改为特定提交哈希或发布版本号。

include(FetchContent)

FetchContent_Declare(
    CabbageHardware
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageHardware.git
    GIT_TAG main # 硬件/平台抽象相关组件（示例：输入/设备适配等）
    GIT_SHALLOW TRUE # 浅克隆加速配置
    EXCLUDE_FROM_ALL # 不主动参与 ALL 构建，由依赖它的目标决定是否编译
)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master # 模型/场景导入库 (OBJ/FBX/GLTF 等)
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master # 单头工具集合 (图像加载 stb_image.h 等)
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG master # ECS (实体组件系统) 框架
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    CabbageConcurrent
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageConcurrent.git
    GIT_TAG main # CabbageConcurrent: 轻量级跨平台并发库 (线程/同步/原子操作等)
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(assimp stb entt CabbageHardware CabbageConcurrent) # 按组一次性拉取并添加子目录
message(STATUS "[3rdparty] Enabled: assimp, stb, entt, CabbageHardware, CabbageConcurrent")

if(CORONA_BUILD_EXAMPLES)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master # 示例窗口/输入管理 (可替换为 SDL/平台层)
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(glfw)
    message(STATUS "[3rdparty] Enabled: glfw (examples)")
endif()
