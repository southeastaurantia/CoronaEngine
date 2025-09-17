# CoronaPython.cmake
# Python 解释器发现与依赖校验功能模块
# 功能概述：
# 1. 首选系统已安装的 Python (满足最小版本要求)；若找不到且允许回退，则尝试使用内置/嵌入式目录。
# 2. 暴露选项控制：
# CORONA_PYTHON_MIN_VERSION            -> 期望的最低 Python 版本 (主.次)
# CORONA_PYTHON_USE_EMBEDDED_FALLBACK  -> 找不到系统 Python 时是否启用 Env/ 下嵌入式回退
# 3. 依赖检查：
# 读取 Misc/requirements.txt，通过辅助脚本 check_pip_modules.py 校验并可按需自动安装缺失包。
# 4. 生成辅助自定义目标：check_python_deps 方便用户手动触发再次校验。
# 设计原则：
# - 配置阶段给出尽可能清晰的问题诊断（缺失解释器/缺失 requirements 文件等）。
# - 避免在未启用自动安装时静默失败，必须明确 FATAL_ERROR 终止提示。

set(CORONA_PYTHON_MIN_VERSION 3.13 CACHE STRING "Minimum required Python3 version (major.minor)")
option(CORONA_PYTHON_USE_EMBEDDED_FALLBACK "Use embedded Env fallback if required version not found system-wide" ON)
set(CORONA_EMBEDDED_PY_DIR "${PROJECT_SOURCE_DIR}/Env/Python-3.13.7" CACHE PATH "Embedded (bundled) Python directory path")

# 步骤一：检测 Python (优先系统，再选择是否回退嵌入式)
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

# Requirements checking -----------------------------------------------------------
option(CORONA_CHECK_PY_DEPS "Check Python dependencies (requirements) during configure" ON)
option(CORONA_AUTO_INSTALL_PY_DEPS "Auto-install missing Python packages during configure" ON)
set(CORONA_PY_REQUIREMENTS_FILE "${PROJECT_SOURCE_DIR}/Misc/requirements.txt")
set(CORONA_PY_CHECK_SCRIPT "${PROJECT_SOURCE_DIR}/Misc/check_pip_modules.py")

function(corona_run_python_requirements_check)
    if(NOT EXISTS "${CORONA_PY_CHECK_SCRIPT}")
    message(WARNING "[Python] Dependency check script missing: ${CORONA_PY_CHECK_SCRIPT}")
        return()
    endif()

    if(NOT EXISTS "${CORONA_PY_REQUIREMENTS_FILE}")
        message(WARNING "[Python] requirements.txt not found: ${CORONA_PY_REQUIREMENTS_FILE}")
        return()
    endif()

    set(_CORONA_PY_CMD "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}")

    if(CORONA_AUTO_INSTALL_PY_DEPS)
        list(APPEND _CORONA_PY_CMD --auto-install --no-unicode)
    else()
        list(APPEND _CORONA_PY_CMD --no-unicode)
    endif()

    message(STATUS "[Python] Running dependency check with interpreter: ${Python3_EXECUTABLE}")
    execute_process(
        COMMAND ${_CORONA_PY_CMD}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _CORONA_PY_RES
        OUTPUT_VARIABLE _CORONA_PY_OUT
        ERROR_VARIABLE _CORONA_PY_ERR
    )

    if(NOT _CORONA_PY_RES EQUAL 0)
        message(STATUS "[Python] Checker stdout:\n${_CORONA_PY_OUT}")

        if(_CORONA_PY_ERR)
            message(STATUS "[Python] Checker stderr:\n${_CORONA_PY_ERR}")
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
