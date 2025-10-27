#include <atomic>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <thread>

#include "corona/framework/runtime/thread_orchestrator.h"
#include "corona/framework/services/time/time_service.h"
#include "test_registry.h"
#include "test_support.h"

void thread_orchestrator_tests() {
    const test_scope test_marker("thread_orchestrator_tests");
    using corona::framework::runtime::thread_orchestrator;

    thread_orchestrator orchestrator;
    assert(!orchestrator.time_service());
    std::atomic<int> counter{0};

    thread_orchestrator::worker_options options{};
    options.tick_interval = std::chrono::milliseconds(1);

    auto handle = orchestrator.add_worker("counter", options, [&](corona::framework::runtime::worker_control& control) {
        ++counter;
        auto time_source = control.time_source();
        assert(!time_source);
        control.request_stop();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    handle.stop();
    assert(counter.load() >= 1);

    auto time_source = corona::framework::services::time::make_time_service();
    orchestrator.set_time_service(time_source);

    std::atomic<int> timed_counter{0};
    auto timed = orchestrator.add_worker("timed", options, [&](corona::framework::runtime::worker_control& control) {
        auto ts = control.time_source();
        assert(ts == time_source);
        ++timed_counter;
        control.request_stop();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    timed.stop();
    assert(timed_counter.load() >= 1);

    auto throwing = orchestrator.add_worker("throwing", options, [&](corona::framework::runtime::worker_control& control) {
        assert(control.time_source() == time_source);
        throw std::runtime_error("intentional");
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    assert(throwing.last_exception() != nullptr);
    throwing.stop();

    orchestrator.stop_all();
    assert(orchestrator.time_service() == time_source);
}
