
cmake_minimum_required(VERSION 4.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =====================================================================================
# Project Metadata
# =====================================================================================
project(
    CoronaEngine
    VERSION 0.5.0
    HOMEPAGE_URL "https://github.com/CoronaEngine/CoronaEngine"
    LANGUAGES C CXX
)

# =====================================================================================
# Functions (Runtime dependency staging)
# =====================================================================================
function(corona_install_runtime_deps target_name)
    get_target_property(_CORONA_DEPS CoronaEngine INTERFACE_CORONA_RUNTIME_DEPS)

    if(NOT _CORONA_DEPS)
        message(WARNING "CoronaEngine library did not specify any runtime dependencies.")
        return()
    endif()

    message(STATUS "Scheduling runtime dependencies for ${target_name}: ${_CORONA_DEPS}")
    set(_DESTINATION_DIR "$<TARGET_FILE_DIR:${target_name}>")
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${_CORONA_DEPS}
        "${_DESTINATION_DIR}"
        COMMENT "Copying Corona runtime dependencies to ${target_name} output directory"
        VERBATIM
    )
endfunction()



# =====================================================================================
# Policies / Global Options
# =====================================================================================
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Examples option (new name). If old variable defined, mirror & warn.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(CORONA_BUILD_EXAMPLES "Build examples" ON)
else()
    option(CORONA_BUILD_EXAMPLES "Build examples" OFF)
endif()


# =====================================================================================
# Python Detection Options
# =====================================================================================
set(CORONA_PYTHON_MIN_VERSION 3.13 CACHE STRING "Minimum required Python3 version (major.minor)")
option(CORONA_PYTHON_USE_EMBEDDED_FALLBACK "Allow fallback to embedded Env/Python-<version> if system Python not found" ON)
set(CORONA_EMBEDDED_PY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Env/Python-3.13.7" CACHE PATH "Embedded Python directory")

# =====================================================================================
# Python Detection (System first then embedded fallback)
# =====================================================================================
if(DEFINED Python3_EXECUTABLE AND EXISTS "${Python3_EXECUTABLE}")
    message(STATUS "Using user-specified Python3_EXECUTABLE=${Python3_EXECUTABLE}")
    find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
else()
    find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)

    if(NOT Python3_FOUND)
        if(CORONA_PYTHON_USE_EMBEDDED_FALLBACK)
            if(WIN32)
                set(_CORONA_EMBEDDED_EXE "${CORONA_EMBEDDED_PY_DIR}/python.exe")
            else()
                set(_CORONA_EMBEDDED_EXE "${CORONA_EMBEDDED_PY_DIR}/bin/python3")
            endif()

            if(EXISTS "${_CORONA_EMBEDDED_EXE}")
                set(Python3_EXECUTABLE "${_CORONA_EMBEDDED_EXE}" CACHE FILEPATH "Embedded Python executable" FORCE)
                message(STATUS "System Python ${CORONA_PYTHON_MIN_VERSION} not found, fallback to embedded: ${Python3_EXECUTABLE}")
                find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
            else()
                message(FATAL_ERROR "System Python >=${CORONA_PYTHON_MIN_VERSION} not found and embedded interpreter missing at ${_CORONA_EMBEDDED_EXE}")
            endif()
        else()
            message(FATAL_ERROR "System Python >=${CORONA_PYTHON_MIN_VERSION} not found and fallback disabled (-DCORONA_PYTHON_USE_EMBEDDED_FALLBACK=ON to enable).")
        endif()
    endif()
endif()

if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "Python interpreter resolution failed.")
endif()

message(STATUS "Chosen Python interpreter: ${Python3_EXECUTABLE}")

# =====================================================================================
# Python Requirements Check
# =====================================================================================
option(CORONA_CHECK_PY_DEPS "Check Python requirements during CMake configure" ON)
option(CORONA_AUTO_INSTALL_PY_DEPS "Auto install missing python packages during configure" ON)

set(CORONA_PY_REQUIREMENTS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Misc/requirements.txt")
set(CORONA_PY_CHECK_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/Misc/check_pip_modules.py")

function(corona_run_python_requirements_check)
    if(NOT EXISTS "${CORONA_PY_CHECK_SCRIPT}")
        message(WARNING "Python dependency check script not found: ${CORONA_PY_CHECK_SCRIPT}")
        return()
    endif()

    if(NOT EXISTS "${CORONA_PY_REQUIREMENTS_FILE}")
        message(WARNING "Python requirements file not found: ${CORONA_PY_REQUIREMENTS_FILE}")
        return()
    endif()

    set(_CORONA_PY_CMD "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}")

    if(CORONA_AUTO_INSTALL_PY_DEPS)
        list(APPEND _CORONA_PY_CMD --auto-install --no-unicode)
    else()
        list(APPEND _CORONA_PY_CMD --no-unicode)
    endif()

    message(STATUS "Checking Python dependencies with: ${Python3_EXECUTABLE}")
    execute_process(
        COMMAND ${_CORONA_PY_CMD}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE _CORONA_PY_RES
        OUTPUT_VARIABLE _CORONA_PY_OUT
        ERROR_VARIABLE _CORONA_PY_ERR
    )

    if(NOT _CORONA_PY_RES EQUAL 0)
        message(STATUS "Python dependency check output:\n${_CORONA_PY_OUT}")

        if(_CORONA_PY_ERR)
            message(STATUS "Python dependency check stderr:\n${_CORONA_PY_ERR}")
        endif()

        message(FATAL_ERROR "Python dependencies check failed (exit code ${_CORONA_PY_RES}).")
    else()
        message(STATUS "Python dependencies satisfied for interpreter: ${Python3_EXECUTABLE}")
    endif()
endfunction()

if(CORONA_CHECK_PY_DEPS)
    corona_run_python_requirements_check()
endif()

add_custom_target(
    check_python_deps
    COMMAND "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}" --no-unicode
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Manual Python dependencies check using ${Python3_EXECUTABLE}"
    VERBATIM
)

add_compile_definitions(
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    NOMINMAX
    ENTT_ID_TYPE=uint64_t # 定义 ENTT 的 ID 类型为 uint64_t
    ENTT_USE_ATOMIC # 使用 ENTT 原子操作
    FMT_HEADER_ONLY=1 # 使用 fmt header-only 版本
    CORONA_PYTHON_EXE="${Python3_EXECUTABLE}"

    $<$<CONFIG:Debug>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:CORONA_ENGINE_DEBUG>
    $<$<CONFIG:Release>:CORONA_ENGINE_RELEASE>
    $<$<CONFIG:MinSizeRel>:CORONA_ENGINE_RELEASE>
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

# FetchContent_Declare(
# oneTBB
# GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
# GIT_TAG master
# EXCLUDE_FROM_ALL
# )
# FetchContent_MakeAvailable(oneTBB)
add_subdirectory(Src)

if(CORONA_BUILD_EXAMPLES)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(glfw)

    add_subdirectory(Examples)
endif()

# =====================================================================================
# Summary
# =====================================================================================
message(STATUS "================ CoronaEngine Configuration Summary ==============")
message(STATUS "  Python Executable : ${Python3_EXECUTABLE}")

if(Python3_VERSION)
    message(STATUS "  Python Version    : ${Python3_VERSION}")
endif()

message(STATUS "  Build Examples    : ${CORONA_BUILD_EXAMPLES}")
message(STATUS "  Check Py Deps     : ${CORONA_CHECK_PY_DEPS}")
message(STATUS "  Auto Install Deps : ${CORONA_AUTO_INSTALL_PY_DEPS}")
message(STATUS "===============================================================")