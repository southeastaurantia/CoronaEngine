#include "test_registry.h"
#include "test_support.h"

#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "corona/framework/plugin/plugin_manifest.h"
#include "corona/framework/runtime/runtime_coordinator.h"
#include "corona/framework/runtime/system.h"

namespace {

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

}  // namespace

void runtime_dependency_tests() {
    const test_scope test_marker("runtime_dependency_tests");
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
