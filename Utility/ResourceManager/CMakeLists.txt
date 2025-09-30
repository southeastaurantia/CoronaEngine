corona_add_utility(
    NAME CoronaResourceManager
    TYPE STATIC
    DESCRIPTION "Corona Engine resource management system with TBB integration"
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

# 特殊配置：添加TBB和spdlog包含目录
target_include_directories(CoronaResourceManager
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/include"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/Env/spdlog-1.15.3/include"
)

# 更正别名（因为目标名与原来不同）
add_library(CoronaResources ALIAS CoronaResourceManager)
