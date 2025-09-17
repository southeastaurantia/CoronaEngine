# CoronaPython.cmake
# Python interpreter detection and requirements checking utilities

set(CORONA_PYTHON_MIN_VERSION 3.13 CACHE STRING "Minimum required Python3 version (major.minor)")
option(CORONA_PYTHON_USE_EMBEDDED_FALLBACK "Allow fallback to embedded Env/Python-<version> if system Python not found" ON)
set(CORONA_EMBEDDED_PY_DIR "${CMAKE_SOURCE_DIR}/Env/Python-3.13.7" CACHE PATH "Embedded Python directory")

# Detect Python (system first)
if(DEFINED Python3_EXECUTABLE AND EXISTS "${Python3_EXECUTABLE}")
    message(STATUS "[Python] Using user-specified Python3_EXECUTABLE=${Python3_EXECUTABLE}")
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
                message(STATUS "[Python] System Python ${CORONA_PYTHON_MIN_VERSION} not found, fallback: ${Python3_EXECUTABLE}")
                find_package(Python3 ${CORONA_PYTHON_MIN_VERSION} COMPONENTS Interpreter Development)
            else()
                message(FATAL_ERROR "[Python] System Python >=${CORONA_PYTHON_MIN_VERSION} not found and embedded interpreter missing at ${_CORONA_EMBEDDED_EXE}")
            endif()
        else()
            message(FATAL_ERROR "[Python] System Python >=${CORONA_PYTHON_MIN_VERSION} not found and fallback disabled.")
        endif()
    endif()
endif()

if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "[Python] Interpreter resolution failed.")
endif()

message(STATUS "[Python] Chosen interpreter: ${Python3_EXECUTABLE}")

# Requirements checking -----------------------------------------------------------
option(CORONA_CHECK_PY_DEPS "Check Python requirements during CMake configure" ON)
option(CORONA_AUTO_INSTALL_PY_DEPS "Auto install missing python packages during configure" ON)
set(CORONA_PY_REQUIREMENTS_FILE "${CMAKE_SOURCE_DIR}/Misc/requirements.txt")
set(CORONA_PY_CHECK_SCRIPT     "${CMAKE_SOURCE_DIR}/Misc/check_pip_modules.py")

function(corona_run_python_requirements_check)
    if(NOT EXISTS "${CORONA_PY_CHECK_SCRIPT}")
        message(WARNING "[Python] Dependency check script not found: ${CORONA_PY_CHECK_SCRIPT}")
        return()
    endif()
    if(NOT EXISTS "${CORONA_PY_REQUIREMENTS_FILE}")
        message(WARNING "[Python] Requirements file not found: ${CORONA_PY_REQUIREMENTS_FILE}")
        return()
    endif()

    set(_CORONA_PY_CMD "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}")
    if(CORONA_AUTO_INSTALL_PY_DEPS)
        list(APPEND _CORONA_PY_CMD --auto-install --no-unicode)
    else()
        list(APPEND _CORONA_PY_CMD --no-unicode)
    endif()

    message(STATUS "[Python] Checking dependencies using: ${Python3_EXECUTABLE}")
    execute_process(
        COMMAND ${_CORONA_PY_CMD}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _CORONA_PY_RES
        OUTPUT_VARIABLE _CORONA_PY_OUT
        ERROR_VARIABLE  _CORONA_PY_ERR
    )

    if(NOT _CORONA_PY_RES EQUAL 0)
        message(STATUS "[Python] Checker stdout:\n${_CORONA_PY_OUT}")
        if(_CORONA_PY_ERR)
            message(STATUS "[Python] Checker stderr:\n${_CORONA_PY_ERR}")
        endif()
        message(FATAL_ERROR "[Python] Dependency check failed (exit ${_CORONA_PY_RES}).")
    else()
        message(STATUS "[Python] Dependencies satisfied.")
    endif()
endfunction()

if(CORONA_CHECK_PY_DEPS)
    corona_run_python_requirements_check()
endif()

add_custom_target(
    check_python_deps
    COMMAND "${Python3_EXECUTABLE}" "${CORONA_PY_CHECK_SCRIPT}" -r "${CORONA_PY_REQUIREMENTS_FILE}" --no-unicode
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "[Python] Manual dependency check"
    VERBATIM
)
