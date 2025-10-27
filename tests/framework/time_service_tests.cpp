#include "test_registry.h"
#include "test_support.h"

#include <cassert>
#include <chrono>
#include <thread>

#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"
#include "corona/framework/services/time/time_service.h"

namespace timing = corona::framework::services::time;

void time_service_tests() {
    const test_scope test_marker("time_service_tests");
    corona::framework::service::service_collection collection;

    auto registered = timing::register_time_service(collection);
    assert(registered);

    auto provider = collection.build_service_provider();
    auto resolved = provider.get_service<timing::time_service>();
    assert(resolved == registered);

    auto snapshot0 = resolved->snapshot();
    assert(snapshot0.frame_index == 0);
    assert(snapshot0.delta_time == timing::steady_clock::duration::zero());
    assert(snapshot0.elapsed_time == timing::steady_clock::duration::zero());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    resolved->advance_frame();

    auto snapshot1 = resolved->snapshot();
    assert(snapshot1.frame_index == 1);
    assert(snapshot1.delta_time >= timing::steady_clock::duration::zero());
    assert(snapshot1.elapsed_time >= snapshot1.delta_time);

    auto since_start = resolved->time_since_start();
    assert(since_start == snapshot1.elapsed_time);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto explicit_timestamp = timing::steady_clock::now();
    resolved->advance_frame(explicit_timestamp);

    auto snapshot2 = resolved->snapshot();
    assert(snapshot2.frame_index == 2);
    assert(snapshot2.current_time >= explicit_timestamp);
    assert(snapshot2.delta_time >= timing::steady_clock::duration::zero());
}
