include_guard(GLOBAL)

set(_CORONA_THREAD_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(Thread ${_CORONA_THREAD_DIR})

add_library(CoronaThread INTERFACE)
add_library(corona::thread ALIAS CoronaThread)

if(CORONA_THREAD_PRIVATE_SOURCES)
    target_sources(CoronaThread INTERFACE ${CORONA_THREAD_PRIVATE_SOURCES})
endif()

if(CORONA_THREAD_PUBLIC_HEADERS)
    target_sources(CoronaThread INTERFACE ${CORONA_THREAD_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaThread INTERFACE cxx_std_20)

target_include_directories(CoronaThread
    INTERFACE
    ${_CORONA_THREAD_DIR}/public
)

target_link_libraries(CoronaThread
    INTERFACE
    corona::utility::logger
    cabbage::concurrent
)
