# ==============================================================================
# CoronaEngine - Source Library
#
# 构建引擎核心库和系统模块
# ==============================================================================

# 添加系统子目录
add_subdirectory(systems/src/animation)
add_subdirectory(systems/src/audio)
add_subdirectory(systems/src/display)
add_subdirectory(systems/src/rendering)

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
    corona::system::animation
    corona::system::audio
    corona::system::display
    corona::system::rendering
)

# 设置 C++ 标准
target_compile_features(CoronaEngine PUBLIC cxx_std_20)

message(STATUS "[CoronaEngine] Core engine library configured")
