include_guard(GLOBAL)

set(_CORONA_SYSTEM_ANIMATION_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(SystemAnimation ${_CORONA_SYSTEM_ANIMATION_DIR})

add_library(CoronaSystemAnimation STATIC)
add_library(corona::system::animation ALIAS CoronaSystemAnimation)

if(CORONA_SYSTEMANIMATION_PRIVATE_SOURCES)
    target_sources(CoronaSystemAnimation PRIVATE ${CORONA_SYSTEMANIMATION_PRIVATE_SOURCES})
endif()

if(CORONA_SYSTEMANIMATION_PUBLIC_HEADERS)
    target_sources(CoronaSystemAnimation PUBLIC ${CORONA_SYSTEMANIMATION_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaSystemAnimation PUBLIC cxx_std_20)

target_include_directories(CoronaSystemAnimation
    PUBLIC
        ${_CORONA_SYSTEM_ANIMATION_DIR}/public
)

target_link_libraries(CoronaSystemAnimation
    PUBLIC
    corona::system::interface
    corona::core

    VisionScene::Resource
)
