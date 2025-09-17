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

set(CORONA_PYTHON_MIN_VERSION 3.13 CACHE STRING "所需的最低 Python3 版本 (主.次)")
option(CORONA_PYTHON_USE_EMBEDDED_FALLBACK "若系统未找到指定版本 Python 是否回退到内置 Env 目录" ON)
set(CORONA_EMBEDDED_PY_DIR "${PROJECT_SOURCE_DIR}/Env/Python-3.13.7" CACHE PATH "嵌入式(内置) Python 目录路径")

# 步骤一：检测 Python (优先系统，再选择是否回退嵌入式)
if(DEFINED Python3_EXECUTABLE AND EXISTS "${Python3_EXECUTABLE}")
    message(STATUS "[Python] 使用用户显式指定的解释器: ${Python3_EXECUTABLE}")
    find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
else()
    find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)

    if(NOT Python3_FOUND)
        if(CORONA_PYTHON_USE_EMBEDDED_FALLBACK)
            set(Python3_ROOT_DIR "${CORONA_EMBEDDED_PY_DIR}" CACHE FILEPATH "Embedded Python executable" FORCE)
            message(STATUS "[Python] 系统未找到满足版本的 Python，使用嵌入式回退: ${Python3_EXECUTABLE}")
            find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
        else()
            message(FATAL_ERROR "[Python] 未找到系统 Python(>=${CORONA_PYTHON_MIN_VERSION}) 且回退被禁用(CORONA_PYTHON_USE_EMBEDDED_FALLBACK=OFF)")
        endif()
    endif()
endif()

if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "[Python] 解释器解析失败，无法继续")
endif()

message(STATUS "[Python] 最终选用解释器: ${Python3_EXECUTABLE}")

# Requirements checking -----------------------------------------------------------
option(CORONA_CHECK_PY_DEPS "在配置阶段检查 Python 依赖 (requirements)" ON)
option(CORONA_AUTO_INSTALL_PY_DEPS "在配置阶段自动安装缺失的 Python 包" ON)
set(CORONA_PY_REQUIREMENTS_FILE "${CMAKE_SOURCE_DIR}/Misc/requirements.txt")
set(CORONA_PY_CHECK_SCRIPT "${CMAKE_SOURCE_DIR}/Misc/check_pip_modules.py")

function(corona_run_python_requirements_check)
    if(NOT EXISTS "${CORONA_PY_CHECK_SCRIPT}")
        message(WARNING "[Python] 依赖检查脚本缺失: ${CORONA_PY_CHECK_SCRIPT}")
        return()
    endif()

    if(NOT EXISTS "${CORONA_PY_REQUIREMENTS_FILE}")
        message(WARNING "[Python] requirements.txt 未找到: ${CORONA_PY_REQUIREMENTS_FILE}")
        return()
    endif()

    set(_CORONA_PY_CMD "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}")

    if(CORONA_AUTO_INSTALL_PY_DEPS)
        list(APPEND _CORONA_PY_CMD --auto-install --no-unicode)
    else()
        list(APPEND _CORONA_PY_CMD --no-unicode)
    endif()

    message(STATUS "[Python] 正在使用解释器执行依赖校验: ${Python3_EXECUTABLE}")
    execute_process(
        COMMAND ${_CORONA_PY_CMD}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _CORONA_PY_RES
        OUTPUT_VARIABLE _CORONA_PY_OUT
        ERROR_VARIABLE _CORONA_PY_ERR
    )

    if(NOT _CORONA_PY_RES EQUAL 0)
        message(STATUS "[Python] 校验脚本标准输出:\n${_CORONA_PY_OUT}")

        if(_CORONA_PY_ERR)
            message(STATUS "[Python] 校验脚本标准错误:\n${_CORONA_PY_ERR}")
        endif()

        message(FATAL_ERROR "[Python] 依赖校验失败 (退出码 ${_CORONA_PY_RES})，请查看上方输出并修复后重试")
    else()
        message(STATUS "[Python] 依赖满足，无需额外安装")
    endif()
endfunction()

if(CORONA_CHECK_PY_DEPS)
    corona_run_python_requirements_check()
endif()

add_custom_target(
    check_python_deps
    COMMAND "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}" --no-unicode
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "[Python] 手动触发依赖检查"
    VERBATIM
)
