add_library(CoronaLogger STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Log.h
)

# 对外暴露本库自己的 include 目录 (PUBLIC)，这样链接 CoronaLogger（或传递链接 CoronaEngine） 的外部目标可以直接 #include <Log.h>
# Log.h 直接引用 spdlog，因此也需要向外传递其 include 目录。
target_include_directories(CoronaLogger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        "${PROJECT_SOURCE_DIR}/Env/spdlog-1.15.3/include"
)
