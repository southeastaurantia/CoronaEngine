include_guard(GLOBAL)

set(_CORONA_UTILITY_RESOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(UtilityResource ${_CORONA_UTILITY_RESOURCE_DIR})

add_library(CoronaUtilityResource STATIC)
add_library(corona::utility::resource ALIAS CoronaUtilityResource)

if(CORONA_UTILITYRESOURCE_PRIVATE_SOURCES)
    target_sources(CoronaUtilityResource PRIVATE ${CORONA_UTILITYRESOURCE_PRIVATE_SOURCES})
endif()

if(CORONA_UTILITYRESOURCE_PUBLIC_HEADERS)
    target_sources(CoronaUtilityResource PUBLIC ${CORONA_UTILITYRESOURCE_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaUtilityResource PUBLIC cxx_std_20)

target_include_directories(CoronaUtilityResource
    PUBLIC
    ${_CORONA_UTILITY_RESOURCE_DIR}/public
)

target_link_libraries(CoronaUtilityResource
    PUBLIC
    corona::utility::logger
    cabbage::concurrent
    VisionScene::Resource
)