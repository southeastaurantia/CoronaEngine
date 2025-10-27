#include "test_registry.h"
#include "test_support.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <string_view>
#include <thread>

#include "corona/framework/plugin/plugin_manifest.h"
#include "corona/framework/runtime/runtime_coordinator.h"
#include "corona/framework/runtime/system.h"
#include "corona/framework/service/service_collection.h"

namespace {

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

}  // namespace

void runtime_coordinator_tests() {
    const test_scope test_marker("runtime_coordinator_tests");
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
