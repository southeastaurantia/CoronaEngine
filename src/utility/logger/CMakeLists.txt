corona_add_utility(
    NAME CoronaLogger
    TYPE STATIC
    DESCRIPTION "Corona Engine logging system with spdlog integration"
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp
    PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Log.h
)

# 特殊配置：添加spdlog包含目录
target_include_directories(CoronaLogger
    PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party/spdlog-1.15.3/include"
)
