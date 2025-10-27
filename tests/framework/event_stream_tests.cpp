#include <cassert>
#include <chrono>

#include "corona/framework/messaging/event_stream.h"
#include "test_registry.h"
#include "test_support.h"

void event_stream_tests() {
    const test_scope test_marker("event_stream_tests");
    corona::framework::messaging::event_stream<int> stream;
    auto sub = stream.subscribe();
    stream.publish(42);

    int value = 0;
    auto popped = sub.try_pop(value);
    assert(popped);
    assert(value == 42);

    stream.publish(100);
    auto popped_again = sub.wait_for(value, std::chrono::milliseconds(10));
    assert(popped_again);
    assert(value == 100);

    sub.close();
}
