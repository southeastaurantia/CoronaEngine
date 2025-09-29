add_library(CoronaConcurrent STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/atomic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/thread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/concurrent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/concurrent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/atomic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/memory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/sync.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core/thread.h
)

# 对外暴露本库的 include 目录 (PUBLIC)，这样链接 CoronaConcurrent 的外部目标可以直接 #include <concurrent.h>
target_include_directories(CoronaConcurrent
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接系统库
if(WIN32)
    target_link_libraries(CoronaConcurrent PRIVATE synchronization)
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(CoronaConcurrent PRIVATE Threads::Threads)
endif()