#include "test_registry.h"
#include "test_support.h"

#include <cassert>
#include <string>

#include "corona/framework/messaging/messaging_hub.h"

void messaging_hub_tests() {
    const test_scope test_marker("messaging_hub_tests");
    corona::framework::messaging::messaging_hub hub;

    auto stream_a = hub.acquire_event_stream<int>("numbers");
    auto stream_b = hub.acquire_event_stream<int>("numbers");
    assert(stream_a == stream_b);

    auto channel_a = hub.acquire_command_channel<int, int>("doubles");
    auto channel_b = hub.acquire_command_channel<int, int>("doubles");
    assert(channel_a == channel_b);

    auto projection_a = hub.acquire_projection<std::string>("status");
    auto projection_b = hub.acquire_projection<std::string>("status");
    assert(projection_a == projection_b);
}
