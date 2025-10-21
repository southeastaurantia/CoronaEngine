cmake_minimum_required(VERSION 3.24)

add_library(arch_framework STATIC
    src/service_descriptor.cpp
    src/service_provider.cpp
    src/service_collection.cpp
)

set_target_properties(arch_framework PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_include_directories(arch_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(arch_framework PUBLIC cxx_std_20)

add_executable(arch_framework_tests
    tests/service_and_messaging_tests.cpp
)

target_link_libraries(arch_framework_tests
    PRIVATE
        arch_framework
)

target_include_directories(arch_framework_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME arch_framework_tests COMMAND arch_framework_tests)
