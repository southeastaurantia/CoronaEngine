add_library(CoronaResources STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ResourceTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BinaryResource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextResource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ResourceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ResourceTypes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/IResource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/IResourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/BinaryResource.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TextResource.h
)

target_include_directories(CoronaResources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        "${PROJECT_SOURCE_DIR}/Env/oneapi-tbb-2022.2.0/include"
    PRIVATE
        ${PROJECT_SOURCE_DIR}/Utility/Logger/include
        "${PROJECT_SOURCE_DIR}/Env/spdlog-1.15.3/include" # 使用 Log.h 需要 fmt bundled 头
)

target_link_libraries(CoronaResources
    PUBLIC
        CoronaLogger
)
