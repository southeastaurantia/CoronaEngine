include_guard(GLOBAL)

set(_CORONA_INTERFACE_HEADERS
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/interfaces/ISystem.h
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/interfaces/ThreadedSystem.h
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/interfaces/Concurrency.h
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/interfaces/ServiceLocator.h
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/interfaces/SystemContext.h
    ${CORONA_ENGINE_PUBLIC_INCLUDE_ROOT}/corona/interfaces/Services.h
)

add_library(CoronaInterfaces INTERFACE)
add_library(corona::interfaces ALIAS CoronaInterfaces)

target_sources(CoronaInterfaces
    INTERFACE
        ${_CORONA_INTERFACE_HEADERS}
)

corona_target_use_public_includes(CoronaInterfaces SCOPE INTERFACE)
