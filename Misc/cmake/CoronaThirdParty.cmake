# CoronaThirdParty.cmake
# FetchContent declarations for external dependencies

include(FetchContent)

FetchContent_Declare(
    CabbageHardware
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageHardware.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(assimp stb entt CabbageHardware)

if(CORONA_BUILD_EXAMPLES)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(glfw)
endif()
