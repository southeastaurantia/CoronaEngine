#include <cassert>
#include <future>

#include "corona/framework/messaging/command_channel.h"
#include "test_registry.h"
#include "test_support.h"

void command_channel_tests() {
    const test_scope test_marker("command_channel_tests");
    corona::framework::messaging::command_channel<int, int> channel;
    channel.register_handler([](int x) {
        return x * 2;
    });

    auto future = channel.send_async(21);
    assert(future.get() == 42);

    auto value = channel.send_sync(10);
    assert(value == 20);

    channel.reset_handler();
}
