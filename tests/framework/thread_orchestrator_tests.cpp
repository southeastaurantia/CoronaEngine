#include "test_registry.h"
#include "test_support.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "corona/framework/runtime/thread_orchestrator.h"

void thread_orchestrator_tests() {
    const test_scope test_marker("thread_orchestrator_tests");
    using corona::framework::runtime::thread_orchestrator;

    thread_orchestrator orchestrator;
    std::atomic<int> counter{0};

    thread_orchestrator::worker_options options{};
    options.tick_interval = std::chrono::milliseconds(1);

    auto handle = orchestrator.add_worker("counter", options, [&](corona::framework::runtime::worker_control& control) {
        ++counter;
        control.request_stop();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    handle.stop();
    assert(counter.load() >= 1);

    auto throwing = orchestrator.add_worker("throwing", options, [&](corona::framework::runtime::worker_control&) {
        throw std::runtime_error("intentional");
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    assert(throwing.last_exception() != nullptr);
    throwing.stop();

    orchestrator.stop_all();
}
