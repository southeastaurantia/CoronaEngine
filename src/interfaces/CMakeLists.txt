include_guard(GLOBAL)

set(_CORONA_INTERFACE_ROOT ${PROJECT_SOURCE_DIR}/include)

set(_CORONA_INTERFACE_HEADERS
    ${_CORONA_INTERFACE_ROOT}/corona/interfaces/ISystem.h
    ${_CORONA_INTERFACE_ROOT}/corona/interfaces/ThreadedSystem.h
    ${_CORONA_INTERFACE_ROOT}/corona/interfaces/Concurrency.h
    ${_CORONA_INTERFACE_ROOT}/corona/interfaces/ServiceLocator.h
    ${_CORONA_INTERFACE_ROOT}/corona/interfaces/SystemContext.h
    ${_CORONA_INTERFACE_ROOT}/corona/interfaces/Services.h
)

add_library(CoronaInterfaces INTERFACE)
add_library(corona::interfaces ALIAS CoronaInterfaces)

target_sources(CoronaInterfaces
    INTERFACE
        ${_CORONA_INTERFACE_HEADERS}
)

target_include_directories(CoronaInterfaces
    INTERFACE
        $<BUILD_INTERFACE:${_CORONA_INTERFACE_ROOT}>
)
