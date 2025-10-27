#include <chrono>
#include <iostream>

#include "test_registry.h"

int main() {
    const auto suite_start = std::chrono::steady_clock::now();
    std::cout << "[SUITE][START] corona::framework smoke tests" << std::endl;

    service_container_tests();
    logging_service_tests();
    logging_performance_tests();
    logging_multithread_performance_tests();
    time_service_tests();
    event_stream_tests();
    command_channel_tests();
    data_projection_tests();
    messaging_hub_tests();
    thread_orchestrator_tests();
    plugin_manifest_tests();
    plugin_manifest_failure_tests();
    plugin_manifest_file_example();
    runtime_coordinator_tests();
    runtime_dependency_tests();

    const auto suite_end = std::chrono::steady_clock::now();
    const auto suite_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(suite_end - suite_start);
    std::cout << "[SUITE][END] corona::framework smoke tests (" << suite_elapsed.count() << " ms)" << std::endl;
    std::cout << "corona::framework smoke tests passed" << std::endl;
    return 0;
}
