
cmake_minimum_required(VERSION 4.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# =====================================================================================
# Project Metadata
# =====================================================================================
project(CoronaEngine VERSION 0.5.0 HOMEPAGE_URL "https://github.com/CoronaEngine/CoronaEngine" LANGUAGES C CXX)

# --- Modular includes -------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaOptions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaPython.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaRuntimeDeps.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaCompileConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Misc/cmake/CoronaThirdParty.cmake)

add_subdirectory(Src)
if(CORONA_BUILD_EXAMPLES)
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