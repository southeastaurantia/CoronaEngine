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

FetchContent_Declare(CoronaLogger
    GIT_REPOSITORY https://github.com/CoronaEngine/CoronaLogger.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(CabbageConcurrent
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageConcurrent.git
    GIT_TAG main
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

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

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

FetchContent_MakeAvailable(CoronaLogger)
message(STATUS "[3rdparty] CoronaLogger module enabled")

FetchContent_MakeAvailable(CabbageHardware)
message(STATUS "[3rdparty] CabbageHardware module enabled")

FetchContent_MakeAvailable(CabbageConcurrent)
message(STATUS "[3rdparty] CabbageConcurrent module enabled")

FetchContent_MakeAvailable(CoronaResource)
message(STATUS "[3rdparty] CoronaResource module enabled")

if(CORONA_BUILD_VISION)
    FetchContent_MakeAvailable(Vision)
    message(STATUS "[3rdparty] Vision module enabled")
endif()

if(CORONA_BUILD_EXAMPLES)
    FetchContent_MakeAvailable(glfw)
    message(STATUS "[3rdparty] glfw module enabled")
endif()
