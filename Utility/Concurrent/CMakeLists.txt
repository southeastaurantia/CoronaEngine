add_library(CoronaConcurrent INTERFACE)

# 公共头文件路径
set_target_properties(CoronaConcurrent PROPERTIES EXPORT_NAME Concurrent)

target_include_directories(CoronaConcurrent
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(CoronaConcurrent INTERFACE cxx_std_20)
