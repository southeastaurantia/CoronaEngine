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

target_compile_definitions(CoronaFrameworkTest
    PRIVATE
        CORONA_FRAMEWORK_TEST_ASSET_DIR="$<TARGET_FILE_DIR:CoronaFrameworkTest>/assets"
)

add_custom_command(TARGET CoronaFrameworkTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:CoronaFrameworkTest>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/examples/corona_framework/sample_manifest.json"
        "$<TARGET_FILE_DIR:CoronaFrameworkTest>/assets/sample_manifest.json"
)

add_test(NAME CoronaFrameworkTest COMMAND CoronaFrameworkTest)
