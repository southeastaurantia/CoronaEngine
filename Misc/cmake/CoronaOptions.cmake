# CoronaOptions.cmake
# Project-wide options and high-level feature toggles

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# Examples option
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    option(CORONA_BUILD_EXAMPLES "Build examples" ON)
else()
    option(CORONA_BUILD_EXAMPLES "Build examples" OFF)
endif()
