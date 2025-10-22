add_library(CoronaFramework STATIC
    plugin/plugin_manifest.cpp
    runtime/runtime_coordinator.cpp
    runtime/system_descriptor.cpp
    runtime/thread_orchestrator.cpp
    service/service_collection.cpp
    service/service_descriptor.cpp
    service/service_provider.cpp
)

add_library(corona::framework ALIAS CoronaFramework)


target_compile_features(CoronaFramework PUBLIC cxx_std_20)

corona_target_use_public_includes(CoronaFramework SCOPE PRIVATE)