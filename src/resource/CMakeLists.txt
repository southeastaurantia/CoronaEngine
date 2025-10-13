include_guard(GLOBAL)

set(_CORONA_RESOURCE_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(Resource ${_CORONA_RESOURCE_MODULE_DIR})

add_library(CoronaResource STATIC)
add_library(corona::resource ALIAS CoronaResource)

if(CORONA_RESOURCE_PRIVATE_SOURCES)
    target_sources(CoronaResource PRIVATE ${CORONA_RESOURCE_PRIVATE_SOURCES})
endif()

if(CORONA_RESOURCE_PUBLIC_HEADERS)
    target_sources(CoronaResource PUBLIC ${CORONA_RESOURCE_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaResource PUBLIC cxx_std_20)

target_include_directories(CoronaResource
    PUBLIC
    ${_CORONA_RESOURCE_MODULE_DIR}/public
    ${stb_SOURCE_DIR}
)

target_link_directories(CoronaResource
    PUBLIC
    ${stb_SOURCE_DIR}
)

target_link_libraries(CoronaResource
    PUBLIC
    corona::utility::logger
    corona::utility::resource

    assimp::assimp
    CabbageHardware
)