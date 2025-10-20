include_guard(GLOBAL)

set(_CORONA_CORE_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

corona_collect_module(Core ${_CORONA_CORE_MODULE_DIR})

set(_CORONA_CORE_KERNEL_SOURCE ${_CORONA_CORE_MODULE_DIR}/src/EngineKernel.cpp)
list(REMOVE_ITEM CORONA_CORE_PRIVATE_SOURCES ${_CORONA_CORE_KERNEL_SOURCE})

set(_CORONA_CORE_SERVICE_SOURCES
    ${_CORONA_CORE_MODULE_DIR}/src/services/LoggerService.cpp
    ${_CORONA_CORE_MODULE_DIR}/src/services/ResourceServiceAdapter.cpp
    ${_CORONA_CORE_MODULE_DIR}/src/services/CommandSchedulerService.cpp
)
foreach(_svc_src IN LISTS _CORONA_CORE_SERVICE_SOURCES)
    list(REMOVE_ITEM CORONA_CORE_PRIVATE_SOURCES ${_svc_src})
endforeach()

add_library(CoronaCoreKernel STATIC ${_CORONA_CORE_KERNEL_SOURCE})
add_library(corona::core::kernel ALIAS CoronaCoreKernel)

target_compile_features(CoronaCoreKernel PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaCoreKernel)

if(DEFINED ktm_SOURCE_DIR)
    target_include_directories(CoronaCoreKernel
        PUBLIC
            $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
    )
endif()

target_link_libraries(CoronaCoreKernel
    PUBLIC
        corona::interfaces
)

add_library(CoronaCoreServices STATIC ${_CORONA_CORE_SERVICE_SOURCES})
add_library(corona::core::services ALIAS CoronaCoreServices)

target_compile_features(CoronaCoreServices PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaCoreServices)

target_link_libraries(CoronaCoreServices
    PUBLIC
        corona::interfaces
        corona::thread
        CoronaResource::Resource
        Corona::Logger
)

add_library(CoronaCore STATIC)
add_library(corona::core ALIAS CoronaCore)

if(CORONA_CORE_PRIVATE_SOURCES)
    target_sources(CoronaCore PRIVATE ${CORONA_CORE_PRIVATE_SOURCES})
endif()

if(CORONA_CORE_PUBLIC_HEADERS)
    target_sources(CoronaCore PUBLIC ${CORONA_CORE_PUBLIC_HEADERS})
endif()

target_compile_features(CoronaCore PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaCore)

if(DEFINED ktm_SOURCE_DIR)
    target_include_directories(CoronaCore
        PUBLIC
            $<BUILD_INTERFACE:${ktm_SOURCE_DIR}>
    )
endif()

target_link_libraries(CoronaCore
    PUBLIC
        corona::core::kernel
        corona::core::services
        corona::interfaces
        corona::utils
        ktm
        EnTT::EnTT
    PRIVATE
        corona::thread
        CoronaResource::Resource
        Corona::Logger
)
