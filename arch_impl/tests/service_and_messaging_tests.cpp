#include "corona/framework/messaging/command_channel.h"
#include "corona/framework/messaging/data_projection.h"
#include "corona/framework/messaging/event_stream.h"
#include "corona/framework/messaging/messaging_hub.h"
#include "corona/framework/plugin/plugin_manifest.h"
#include "corona/framework/runtime/runtime_coordinator.h"
#include "corona/framework/runtime/system.h"
#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
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

void thread_orchestrator_tests() {
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

struct sample_system final : corona::framework::runtime::system {
    static std::atomic<int> tick_count;

    std::string_view id() const noexcept override {
        return "sample.system";
    }

    void configure(const corona::framework::runtime::system_context& context) override {
        (void)context;
    }

    void start() override {}

    void execute(corona::framework::runtime::worker_control& control) override {
        ++tick_count;
        if (tick_count.load() >= 3) {
            control.request_stop();
        }
        control.sleep_for(std::chrono::milliseconds(1));
    }

    void stop() override {}
};

std::atomic<int> sample_system::tick_count{0};

struct tracking_state {
    static void reset() {
        std::lock_guard<std::mutex> guard(mutex);
        start_order.clear();
    }

    static void record(std::string id) {
        std::lock_guard<std::mutex> guard(mutex);
        start_order.push_back(std::move(id));
    }

    static std::vector<std::string> snapshot() {
        std::lock_guard<std::mutex> guard(mutex);
        return start_order;
    }

    static std::mutex mutex;
    static std::vector<std::string> start_order;
};

std::mutex tracking_state::mutex;
std::vector<std::string> tracking_state::start_order;

struct beta_system final : corona::framework::runtime::system {
    std::string_view id() const noexcept override {
        return "beta.system";
    }

    void configure(const corona::framework::runtime::system_context& context) override {
        (void)context;
    }

    void start() override {
        tracking_state::record(std::string(id()));
    }

    void execute(corona::framework::runtime::worker_control& control) override {
        control.request_stop();
    }

    void stop() override {}
};

struct alpha_system final : corona::framework::runtime::system {
    std::string_view id() const noexcept override {
        return "alpha.system";
    }

    void configure(const corona::framework::runtime::system_context& context) override {
        (void)context;
    }

    void start() override {
        tracking_state::record(std::string(id()));
    }

    void execute(corona::framework::runtime::worker_control& control) override {
        control.request_stop();
    }

    void stop() override {}
};

void plugin_manifest_tests() {
    const std::string json = R"JSON({
        "name": "example",
        "version": "1.0",
        "dependencies": ["core"],
        "systems": [
            {
                "id": "sample.system",
                "factory": "sample.factory",
                "dependencies": ["render"],
                "tags": ["demo"],
                "tick_ms": 8
            }
        ]
    })JSON";

    auto manifest = corona::framework::plugin::parse_manifest(json);
    assert(manifest.name == "example");
    assert(manifest.systems.size() == 1);
    assert(manifest.systems[0].factory == "sample.factory");
    assert(manifest.systems[0].tick_interval.count() == 8);
}

void plugin_manifest_failure_tests() {
    bool threw_missing_id = false;
    try {
        corona::framework::plugin::parse_manifest(R"({"systems":[{"factory":"missing"}]})");
    } catch (const std::exception&) {
        threw_missing_id = true;
    }
    assert(threw_missing_id);
}

void plugin_manifest_file_example() {
    namespace fs = std::filesystem;
    auto sample_path = fs::path(__FILE__).parent_path() / ".." / "examples" / "sample_manifest.json";
    sample_path = fs::weakly_canonical(sample_path);
    auto manifest = corona::framework::plugin::load_manifest(sample_path);
    assert(manifest.name == "arch_sample");
    assert(manifest.systems.size() == 2);
    assert(manifest.systems[1].dependencies.size() == 1);
}

void runtime_coordinator_tests() {
    using namespace corona::framework;

    runtime::runtime_coordinator coordinator;
    service::service_collection services;
    services.add_singleton<int>(std::make_shared<int>(42));
    coordinator.configure_services(std::move(services));

    auto factory = std::make_shared<runtime::default_system_factory<sample_system>>();
    coordinator.register_factory("sample.factory", factory);

    plugin::plugin_manifest manifest;
    manifest.name = "test";
    manifest.systems.push_back({"sample.system", "sample.factory", {}, {}, std::chrono::milliseconds(2)});
    coordinator.register_manifest(std::move(manifest));

    sample_system::tick_count.store(0);
    coordinator.initialize();
    coordinator.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    coordinator.stop();

    assert(sample_system::tick_count.load() >= 1);
}

void runtime_dependency_tests() {
    using namespace corona::framework;

    tracking_state::reset();

    runtime::runtime_coordinator coordinator;
    service::service_collection services;
    coordinator.configure_services(std::move(services));

    auto beta_factory = std::make_shared<runtime::default_system_factory<beta_system>>();
    auto alpha_factory = std::make_shared<runtime::default_system_factory<alpha_system>>();
    coordinator.register_factory("beta.factory", beta_factory);
    coordinator.register_factory("alpha.factory", alpha_factory);

    plugin::plugin_manifest manifest;
    manifest.name = "dependency-example";
    manifest.systems.push_back({"beta.system", "beta.factory", {}, {}, std::chrono::milliseconds(1)});
    manifest.systems.push_back({"alpha.system", "alpha.factory", {"beta.system"}, {}, std::chrono::milliseconds(1)});
    coordinator.register_manifest(std::move(manifest));

    coordinator.initialize();
    coordinator.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    coordinator.stop();

    auto order = tracking_state::snapshot();
    assert(order.size() == 2);
    assert(order.front() == "beta.system");
    assert(order.back() == "alpha.system");
}

}  // namespace

int main() {
    service_container_tests();
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

    std::cout << "corona::framework smoke tests passed" << std::endl;
    return 0;
}
