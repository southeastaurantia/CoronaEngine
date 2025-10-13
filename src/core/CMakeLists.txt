include_guard(GLOBAL)

set(_CORONA_CORE_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(Core ${_CORONA_CORE_MODULE_DIR})

add_library(CoronaCore STATIC)
add_library(corona::core ALIAS CoronaCore)

if(CORONA_CORE_PRIVATE_SOURCES)
    target_sources(CoronaCore PRIVATE ${CORONA_CORE_PRIVATE_SOURCES})
endif()

if(CORONA_CORE_PUBLIC_HEADERS)
    target_sources(CoronaCore PUBLIC ${CORONA_CORE_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaCore PUBLIC cxx_std_20)

target_include_directories(CoronaCore
    PUBLIC
        ${_CORONA_CORE_MODULE_DIR}/public
)

target_link_libraries(CoronaCore
    PUBLIC
    corona::resource
    corona::script::python
    corona::system::animation
    corona::system::rendering
    corona::system::display
    corona::system::audio
    corona::thread
    corona::utils
    
    ktm
)
