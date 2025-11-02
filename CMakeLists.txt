# ============================================================================== 
# CoronaEngine - Root CMake Configuration
#
# Responsibilities:
#   - Coordinate the build using modular helper scripts located in misc/cmake.
#   - Register project-wide options, toolchains, third-party dependencies, and
#     runtime asset staging helpers.
#   - Add subdirectories in module order: src -> engine -> examples.
# ============================================================================== 

cmake_minimum_required(VERSION 4.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/misc/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------------------------
# Project Declaration
# ------------------------------------------------------------------------------
project(
    CoronaEngine
    VERSION      0.5.0
    HOMEPAGE_URL "https://github.com/CoronaEngine/CoronaEngine"
    LANGUAGES    C CXX
)

# ------------------------------------------------------------------------------
# Modular Configuration Includes
# ------------------------------------------------------------------------------
include(corona_options)             # Global build options (editor/examples/runtime switches)
include(corona_python)              # Embedded Python discovery and dependency checks
include(corona_third_party)         # FetchContent declarations for external dependencies
include(corona_compile_config)      # Compiler settings, standards, and shared definitions
include(corona_collect_module)      # Helpers for module source/header discovery
include(corona_runtime_deps)        # Runtime dependency collection and copy helpers
include(corona_editor)              # Editor asset packaging helpers

# ------------------------------------------------------------------------------
# Testing Support
# ------------------------------------------------------------------------------
include(CTest)

# ------------------------------------------------------------------------------
# Subdirectories
# ------------------------------------------------------------------------------
add_subdirectory(src)               # Core engine static library

if(CORONA_BUILD_EXAMPLES)
    add_subdirectory(examples)      # Example applications
endif()

# ------------------------------------------------------------------------------
# Configuration Summary
# ------------------------------------------------------------------------------
message(STATUS "")
message(STATUS "================================================================================")
message(STATUS "  CoronaEngine Configuration Summary")
message(STATUS "================================================================================")
message(STATUS "  Python executable     : ${Python3_EXECUTABLE}")

if(Python3_VERSION)
    message(STATUS "  Python version        : ${Python3_VERSION}")
endif()

message(STATUS "  Build runtime         : ${BUILD_CORONA_RUNTIME}")
message(STATUS "  Build examples        : ${CORONA_BUILD_EXAMPLES}")
message(STATUS "  Build editor          : ${BUILD_CORONA_EDITOR}")
message(STATUS "  Check Python deps     : ${CORONA_CHECK_PY_DEPS}")
message(STATUS "  Auto install deps     : ${CORONA_AUTO_INSTALL_PY_DEPS}")
message(STATUS "================================================================================")
message(STATUS "")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR})