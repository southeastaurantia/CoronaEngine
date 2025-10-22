# Corona Framework standalone example

corona_add_example(
    NAME corona_framework_example
    SOURCES
        main.cpp
)

target_include_directories(corona_framework_example PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(corona_framework_example PRIVATE
    corona::framework
)

set(_corona_framework_manifest "${CMAKE_CURRENT_SOURCE_DIR}/sample_manifest.json")
add_custom_command(
    TARGET corona_framework_example
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_corona_framework_manifest}"
            "$<TARGET_FILE_DIR:corona_framework_example>/sample_manifest.json"
    COMMENT "[corona_framework_example] Copy sample manifest"
    VERBATIM
)
