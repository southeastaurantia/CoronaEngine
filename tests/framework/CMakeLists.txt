add_executable(CoronaFrameworkTest
    service_and_messaging_tests.cpp
)

target_compile_features(CoronaFrameworkTest PRIVATE cxx_std_20)

target_link_libraries(CoronaFrameworkTest
    PRIVATE
        corona::framework
)

target_include_directories(CoronaFrameworkTest
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

add_test(NAME CoronaFrameworkTest COMMAND CoronaFrameworkTest)
