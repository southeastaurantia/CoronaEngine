include_guard(GLOBAL)

set(_CORONA_UTILITY_LOGGER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(UtilityLogger ${_CORONA_UTILITY_LOGGER_DIR})

add_library(CoronaUtilityLogger STATIC)
add_library(corona::utility::logger ALIAS CoronaUtilityLogger)

if(CORONA_UTILITYLOGGER_PRIVATE_SOURCES)
    target_sources(CoronaUtilityLogger PRIVATE ${CORONA_UTILITYLOGGER_PRIVATE_SOURCES})
endif()

if(CORONA_UTILITYLOGGER_PUBLIC_HEADERS)
    target_sources(CoronaUtilityLogger PUBLIC ${CORONA_UTILITYLOGGER_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaUtilityLogger PUBLIC cxx_std_20)

target_include_directories(CoronaUtilityLogger
    PUBLIC
    ${_CORONA_UTILITY_LOGGER_DIR}/public
    ${PROJECT_SOURCE_DIR}/third_party/spdlog-1.15.3/include
)
