# Arch Framework Examples

## Sample Manifest

- `sample_manifest.json`: Demonstrates how to declare two systems (`beta.system` and `alpha.system`) with an explicit dependency chain and per-system tick intervals.

## Minimal Runtime Setup (pseudo-code)

```cpp
#include "corona/framework/runtime/runtime_coordinator.h"
#include "corona/framework/runtime/system.h"
#include "corona/framework/runtime/thread_orchestrator.h"
#include "corona/framework/service/service_collection.h"

using namespace corona::framework;

struct demo_system : runtime::system {
    std::string_view id() const noexcept override { return "demo.system"; }
    void configure(const runtime::system_context& ctx) override { (void)ctx; }
    void start() override {}
    void execute(runtime::worker_control& control) override {
        control.request_stop();
    }
    void stop() override {}
};

int main() {
    runtime::runtime_coordinator coordinator;

    service::service_collection services;
    services.add_singleton<int>(std::make_shared<int>(42));
    coordinator.configure_services(std::move(services));

    auto factory = std::make_shared<runtime::default_system_factory<demo_system>>();
    coordinator.register_factory("demo.factory", factory);

    plugin::plugin_manifest manifest;
    manifest.name = "demo";
    manifest.systems.push_back({"demo.system", "demo.factory", {}, {}, std::chrono::milliseconds(16)});
    coordinator.register_manifest(std::move(manifest));

    coordinator.initialize();
    coordinator.start();
    coordinator.stop();
}
```

This snippet mirrors the smoke tests and can be adapted into a standalone executable when the `arch_framework` library is linked into an application target.
