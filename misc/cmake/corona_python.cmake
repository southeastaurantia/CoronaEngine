# ==============================================================================
# corona_python.cmake
# ==============================================================================
# 功能：Python 解释器发现与依赖校验功能模块
#
# 概述：
#   1. 首选系统已安装的 Python (满足最小版本要求)
#      若找不到且允许回退，则尝试使用内置/嵌入式目录
#   2. 暴露选项控制：
#      - CORONA_PYTHON_MIN_VERSION: 期望的最低 Python 版本 (主.次)
#      - CORONA_PYTHON_USE_EMBEDDED_FALLBACK: 启用嵌入式 Python 回退
#   3. 依赖检查：
#      读取 misc/pytools/requirements.txt，通过 check_pip_modules.py 校验
#      并可按需自动安装缺失包
#   4. 生成辅助自定义目标：check_python_deps 方便用户手动触发再次校验
#
# 设计原则：
#   - 配置阶段给出尽可能清晰的问题诊断
#   - 避免在未启用自动安装时静默失败，必须明确 FATAL_ERROR 终止提示
# ==============================================================================

include_guard(GLOBAL)

# ------------------------------------------------------------------------------
# Python 配置选项
# ------------------------------------------------------------------------------

set(CORONA_PYTHON_MIN_VERSION 3.13 
    CACHE STRING "Minimum required Python3 version (major.minor)")

option(CORONA_PYTHON_USE_EMBEDDED_FALLBACK 
    "Use embedded fallback if required version not found system-wide" 
    ON)

set(CORONA_EMBEDDED_PY_DIR "${PROJECT_SOURCE_DIR}/third_party/Python-3.13.7" 
    CACHE PATH "Embedded (bundled) Python directory path")

# ------------------------------------------------------------------------------
# Python 探测
# ------------------------------------------------------------------------------
# 优先使用用户指定的 Python 根目录
if(DEFINED Python3_ROOT_DIR AND EXISTS "${Python3_ROOT_DIR}")
    message(STATUS "[Python] Using user-specified Python root: ${Python3_ROOT_DIR}")
    find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
else()
    find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)

    if(NOT Python3_FOUND)
        if(CORONA_PYTHON_USE_EMBEDDED_FALLBACK)
            set(Python3_ROOT_DIR "${CORONA_EMBEDDED_PY_DIR}" CACHE FILEPATH "Embedded Python executable" FORCE)
            message(STATUS "[Python] System Python (>=${CORONA_PYTHON_MIN_VERSION}) not found, using embedded fallback: ${Python3_ROOT_DIR}")
            find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
        else()
            message(FATAL_ERROR "[Python] System Python (>=${CORONA_PYTHON_MIN_VERSION}) not found and fallback disabled (CORONA_PYTHON_USE_EMBEDDED_FALLBACK=OFF)")
        endif()
    endif()
endif()

if(NOT Python3_FOUND)
    message(FATAL_ERROR "[Python] Interpreter not found; cannot continue")
endif()

message(STATUS "[Python] Final chosen interpreter: ${Python3_EXECUTABLE}")

# ------------------------------------------------------------------------------
# Python 依赖校验配置
# ------------------------------------------------------------------------------
option(CORONA_CHECK_PY_DEPS 
    "Check Python dependencies (requirements) during configure" 
    ON)

option(CORONA_AUTO_INSTALL_PY_DEPS 
    "Auto-install missing Python packages during configure" 
    ON)

set(CORONA_PY_REQUIREMENTS_FILE "${PROJECT_SOURCE_DIR}/misc/pytools/requirements.txt")
set(CORONA_PY_CHECK_SCRIPT "${PROJECT_SOURCE_DIR}/misc/pytools/check_pip_modules.py")

# ------------------------------------------------------------------------------
# 工具函数：运行 Python 脚本
# ------------------------------------------------------------------------------
function(corona_run_python OUT_RESULT)
    set(options)
    set(oneValueArgs SCRIPT WORKING_DIRECTORY)
    set(multiValueArgs ARGS)
    cmake_parse_arguments(CRP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CRP_SCRIPT)
        message(FATAL_ERROR "corona_run_python: SCRIPT is required")
    endif()

    if(NOT DEFINED Python3_EXECUTABLE)
        message(FATAL_ERROR "corona_run_python: Python3_EXECUTABLE is not defined")
    endif()

    if(NOT CRP_WORKING_DIRECTORY)
        set(CRP_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    endif()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CRP_SCRIPT}" ${CRP_ARGS}
        WORKING_DIRECTORY "${CRP_WORKING_DIRECTORY}"
        RESULT_VARIABLE _CRP_RES
        OUTPUT_VARIABLE _CRP_OUT
        ERROR_VARIABLE _CRP_ERR
    )
    set(${OUT_RESULT} ${_CRP_RES} PARENT_SCOPE)
    set(CORONA_LAST_PY_STDOUT "${_CRP_OUT}" PARENT_SCOPE)
    set(CORONA_LAST_PY_STDERR "${_CRP_ERR}" PARENT_SCOPE)
endfunction()

function(corona_run_python_requirements_check)
    if(NOT EXISTS "${CORONA_PY_CHECK_SCRIPT}")
        message(WARNING "[Python] Dependency check script missing: ${CORONA_PY_CHECK_SCRIPT}")
        return()
    endif()

    if(NOT EXISTS "${CORONA_PY_REQUIREMENTS_FILE}")
        message(WARNING "[Python] requirements.txt not found: ${CORONA_PY_REQUIREMENTS_FILE}")
        return()
    endif()

    # 常规方式调用 Python 检查依赖
    set(_CORONA_PY_CMD "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}")

    if(CORONA_AUTO_INSTALL_PY_DEPS)
        list(APPEND _CORONA_PY_CMD --auto-install --no-unicode)
    else()
        list(APPEND _CORONA_PY_CMD --no-unicode)
    endif()

    message(STATUS "[Python] Running dependency check with interpreter: ${Python3_EXECUTABLE}")
    set(_CRP_ARGS -r "${CORONA_PY_REQUIREMENTS_FILE}" --no-unicode)

    if(CORONA_AUTO_INSTALL_PY_DEPS)
        list(APPEND _CRP_ARGS --auto-install)
    endif()

    corona_run_python(_CORONA_PY_RES SCRIPT "${CORONA_PY_CHECK_SCRIPT}" ARGS ${_CRP_ARGS} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

    if(NOT _CORONA_PY_RES EQUAL 0)
        if(CORONA_LAST_PY_STDOUT)
            message(STATUS "[Python] Checker stdout:\n${CORONA_LAST_PY_STDOUT}")
        endif()

        if(CORONA_LAST_PY_STDERR)
            message(STATUS "[Python] Checker stderr:\n${CORONA_LAST_PY_STDERR}")
        endif()

        message(FATAL_ERROR "[Python] Requirement check failed (exit code ${_CORONA_PY_RES}); see output above, fix issues, then re-run")
    else()
        message(STATUS "[Python] Requirements satisfied; no installation needed")
    endif()
endfunction()

if(CORONA_CHECK_PY_DEPS)
    corona_run_python_requirements_check()
endif()

add_custom_target(
    check_python_deps
    COMMAND "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}" --no-unicode
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "[Python] Manually trigger dependency check"
    VERBATIM
)
