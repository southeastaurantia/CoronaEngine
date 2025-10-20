# SystemHubs sanity check example

corona_add_example(
    NAME system_hubs_sanity
    SOURCES
        main.cpp
)

target_include_directories(system_hubs_sanity PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}
)
