# ============================================================================== 
# corona_third_party.cmake
#
# Purpose:
#   Declare and fetch external dependencies using `FetchContent`.
#
# Notes:
#   - Centralizes source-level dependencies required by the engine and examples.
#   - Uses parallel capable FetchContent at configure time.
#
# Tips:
#   - Pin `GIT_TAG` values to specific commits or release versions to lock
#     dependency versions where stability is preferred.
# ============================================================================== 

include_guard(GLOBAL)

include(FetchContent)

# ------------------------------------------------------------------------------
# Core dependency declarations
# ------------------------------------------------------------------------------
FetchContent_Declare(CabbageHardware
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageHardware.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG v2.9.2
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(CoronaResource
    GIT_REPOSITORY https://github.com/CoronaEngine/CoronaResource.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(Vision
    GIT_REPOSITORY https://github.com/CoronaEngine/Vision.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

# Note: CoronaFramework has been renamed to CoronaEngine and is no longer a separate dependency
# FetchContent_Declare(CoronaFramework
#     GIT_REPOSITORY https://github.com/CoronaEngine/CoronaFramework.git
#     GIT_TAG main
#     GIT_SHALLOW TRUE
#     EXCLUDE_FROM_ALL
# )


# ------------------------------------------------------------------------------
# Fetch and enable dependencies
# ------------------------------------------------------------------------------
FetchContent_MakeAvailable(assimp)
message(STATUS "[3rdparty] assimp module enabled")

FetchContent_MakeAvailable(stb)
message(STATUS "[3rdparty] stb module enabled")

FetchContent_MakeAvailable(entt)
message(STATUS "[3rdparty] entt module enabled")

FetchContent_MakeAvailable(nanobind)
message(STATUS "[3rdparty] nanobind module enabled")

FetchContent_MakeAvailable(CabbageHardware)
message(STATUS "[3rdparty] CabbageHardware module enabled")

FetchContent_MakeAvailable(CoronaResource)
message(STATUS "[3rdparty] CoronaResource module enabled")

# Note: CoronaFramework/CabbageFramework is no longer needed as a separate dependency
# FetchContent_MakeAvailable(CabbageFramework)
# message(STATUS "[3rdparty] CabbageFramework module enabled")

if(CORONA_BUILD_VISION)
    FetchContent_MakeAvailable(Vision)
    message(STATUS "[3rdparty] Vision module enabled")
endif()