corona_add_utility(
    NAME CoronaConcurrent
    TYPE STATIC
    DESCRIPTION "Corona Engine concurrent containers and utilities"
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/core/atomic.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/sync.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/thread.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/concurrent.cpp
    PUBLIC_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/concurrent.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core/atomic.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core/memory.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core/sync.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/core/thread.h
)

# 特殊配置：链接平台相关的系统库
if(WIN32)
    target_link_libraries(CoronaConcurrent PRIVATE synchronization)
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(CoronaConcurrent PRIVATE Threads::Threads)
endif()