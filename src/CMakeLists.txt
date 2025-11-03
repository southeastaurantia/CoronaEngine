# ==============================================================================
# CoronaEngine - Source Library
#
# 构建引擎核心库和系统模块
# ==============================================================================

# 添加系统子目录
add_subdirectory(systems/src/acoustics)
add_subdirectory(systems/src/optics)
add_subdirectory(systems/src/mechanics)
add_subdirectory(systems/src/geometry)
add_subdirectory(systems/src/animation)
add_subdirectory(systems/src/display)

# ==============================================================================
# CoronaEngine 核心库
# ==============================================================================

add_library(CoronaEngine STATIC
    engine.cpp
    shared_data_hub.cpp

    include/corona/engine.h
    include/corona/shared_data_hub.h
)

add_library(corona::engine ALIAS CoronaEngine)

# 设置包含目录
target_include_directories(CoronaEngine PUBLIC
    ${PROJECT_SOURCE_DIR}/src/include
)

# 链接依赖
target_link_libraries(CoronaEngine PUBLIC
    corona::kernel
    corona::system::acoustics
    corona::system::optics
    corona::system::mechanics
    corona::system::geometry
    corona::system::animation
    corona::system::display
)

# 设置 C++ 标准
target_compile_features(CoronaEngine PUBLIC cxx_std_20)

message(STATUS "[CoronaEngine] Core engine library configured")
