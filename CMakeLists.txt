
cmake_minimum_required(VERSION 4.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(
    CabbageFramework
    VERSION 0.5.0
    HOMEPAGE_URL "https://github.com/CoronaEngine/CabbageFramework"
    LANGUAGES C CXX
)

function(cabbage_install_runtime_deps target_name)
    get_target_property(CABBAGE_DEPS CabbageFramework INTERFACE_CABBAGE_RUNTIME_DEPS)

    if(NOT CABBAGE_DEPS)
        message(WARNING "Cabbage library did not specify any runtime dependencies.")
        return()
    endif()

    message(STATUS "Scheduling runtime dependencies for ${target_name}: ${CABBAGE_DEPS}")

    set(DESTINATION_DIR "$<TARGET_FILE_DIR:${target_name}>")

    add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CABBAGE_DEPS}
            "${DESTINATION_DIR}"
            COMMENT "Copying Cabbage runtime dependencies to ${target_name} output directory"
            VERBATIM
    )
endfunction()

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()


option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(CABBAGE_FRAMEWORK_BUILD_EXAMPLES "Build examples" ON)
else()
    option(CABBAGE_FRAMEWORK_BUILD_EXAMPLES "Build examples" OFF)
endif()

add_compile_definitions(
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    NOMINMAX
    ENTT_ID_TYPE=uint64_t # 定义 ENTT 的 ID 类型为 uint64_t
    ENTT_USE_ATOMIC # 使用 ENTT 原子操作

    $<$<CONFIG:Debug>:CABBAGE_ENGINE_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:CABBAGE_ENGINE_DEBUG>
    $<$<CONFIG:Release>:CABBAGE_ENGINE_RELEASE>
    $<$<CONFIG:MinSizeRel>:CABBAGE_ENGINE_RELEASE>
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)

FetchContent_Declare(
    CabbageHardware
    GIT_REPOSITORY https://github.com/CoronaEngine/CabbageHardware.git
    GIT_TAG main
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(CabbageHardware)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(assimp)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG master
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(entt)

#FetchContent_Declare(
#    oneTBB
#    GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
#    GIT_TAG master
#    EXCLUDE_FROM_ALL
#)
#FetchContent_MakeAvailable(oneTBB)

add_subdirectory(Src)

if(CABBAGE_FRAMEWORK_BUILD_EXAMPLES)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(glfw)

    add_subdirectory(Examples)
endif()