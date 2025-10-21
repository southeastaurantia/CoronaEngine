#include "corona/framework/messaging/command_channel.h"
#include "corona/framework/messaging/data_projection.h"
#include "corona/framework/messaging/event_stream.h"
#include "corona/framework/messaging/messaging_hub.h"
#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"

#include <cassert>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct base_service {
    virtual ~base_service() = default;
    virtual std::string name() const = 0;
};

struct derived_service final : base_service {
    std::string payload;

    explicit derived_service(std::string value = "default")
        : payload(std::move(value)) {}

    std::string name() const override {
        return payload;
    }
};

struct dependent_service {
    std::shared_ptr<base_service> base;

    explicit dependent_service(std::shared_ptr<base_service> input)
        : base(std::move(input)) {}
};

void service_container_tests() {
    corona::framework::service::service_collection collection;
    collection.add_singleton<base_service, derived_service>();
    collection.add_scoped<dependent_service>([](corona::framework::service::service_provider& provider) {
        return std::make_shared<dependent_service>(provider.get_service<base_service>());
    });
    collection.add_transient<std::string>([](corona::framework::service::service_provider&) {
        return std::make_shared<std::string>("transient");
    });

    auto provider = collection.build_service_provider();
    auto singleton_a = provider.get_service<base_service>();
    auto singleton_b = provider.get_service<base_service>();
    assert(singleton_a == singleton_b);
    assert(singleton_a->name() == "default");

    auto scoped_provider = provider.create_scope();
    auto scoped_a = scoped_provider.get_service<dependent_service>();
    auto scoped_b = scoped_provider.get_service<dependent_service>();
    assert(scoped_a == scoped_b);
    assert(scoped_a->base == singleton_a);

    auto other_scope = provider.create_scope();
    auto scoped_c = other_scope.get_service<dependent_service>();
    assert(scoped_c != scoped_a);

    auto transient_a = provider.get_service<std::string>();
    auto transient_b = provider.get_service<std::string>();
    assert(transient_a != transient_b);
    assert(*transient_a == "transient");
}

void event_stream_tests() {
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

void command_channel_tests() {
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

void data_projection_tests() {
    corona::framework::messaging::data_projection<std::vector<int>> projection;
    std::vector<std::vector<int>> observed;

    auto subscription = projection.subscribe([&](const std::vector<int>& data) {
        observed.push_back(data);
    });

    projection.set(std::vector<int>{1, 2, 3});
    projection.set(std::vector<int>{4, 5});

    auto snapshot = projection.get_snapshot();
    assert(snapshot.size() == 2);
    assert(snapshot[0] == 4);

    assert(observed.size() == 2);
    assert(observed[0].size() == 3);
    assert(observed[1].size() == 2);

    subscription.release();
}

void messaging_hub_tests() {
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

}  // namespace

int main() {
    service_container_tests();
    event_stream_tests();
    command_channel_tests();
    data_projection_tests();
    messaging_hub_tests();

    std::cout << "corona::framework smoke tests passed" << std::endl;
    std::cin.get();
    return 0;
}
