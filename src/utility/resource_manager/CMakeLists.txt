corona_add_utility(
    NAME CoronaResourceManager
    TYPE STATIC
    DESCRIPTION "Corona Engine resource management system with Corona concurrent integration"
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ResourceTypes.cpp
    PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ResourceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ResourceTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/IResource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/IResourceLoader.h
)

# 条件性链接 Logger（如果可用）
if(TARGET Corona::Logger)
    target_link_libraries(CoronaResourceManager PUBLIC Corona::Logger)
    message(STATUS "  - Linked with Corona::Logger")
else()
    message(STATUS "  - Corona::Logger not available, proceeding without logging")
endif()

target_link_libraries(CoronaResourceManager
    PUBLIC Corona::Concurrent
)

# 特殊配置：添加spdlog包含目录
target_include_directories(CoronaResourceManager
    PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party/spdlog-1.15.3/include"
)

# 更正别名（因为目标名与原来不同）
add_library(CoronaResources ALIAS CoronaResourceManager)
