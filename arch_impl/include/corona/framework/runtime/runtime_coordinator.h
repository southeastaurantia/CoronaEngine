#pragma once

#include "corona/framework/messaging/messaging_hub.h"
#include "corona/framework/plugin/plugin_manifest.h"
#include "corona/framework/runtime/system.h"
#include "corona/framework/runtime/thread_orchestrator.h"
#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace corona::framework::runtime {

class runtime_coordinator {
   public:
    struct configuration {
        std::vector<std::filesystem::path> manifest_paths;
    };

    runtime_coordinator();

    void configure_services(service::service_collection collection);
    void register_descriptor(system_descriptor descriptor);
    void register_factory(std::string name, std::shared_ptr<system_factory> factory);
    void register_manifest(plugin::plugin_manifest manifest);

    void load_manifests(const configuration& config);

    void initialize();
    void start();
    void stop();

    [[nodiscard]] messaging::messaging_hub& messaging() noexcept { return messaging_hub_; }
    [[nodiscard]] thread_orchestrator& orchestrator() noexcept { return orchestrator_; }

   private:
    void ensure_not_running() const;
    void hydrate_manifests();
    void resolve_dependency_order();

    const system_descriptor& descriptor_for(const std::string& id) const;

    bool initialized_ = false;
    bool running_ = false;

    std::unique_ptr<service::service_collection> service_collection_;
    std::optional<service::service_provider> root_provider_;
    messaging::messaging_hub messaging_hub_;
    thread_orchestrator orchestrator_;

    std::unordered_map<std::string, std::shared_ptr<system_factory>> factories_;
    std::unordered_map<std::string, system_descriptor> descriptors_;
    std::vector<std::string> startup_order_;
    std::vector<plugin::plugin_manifest> manifests_;

    struct active_system {
        std::string id;
        std::unique_ptr<system> instance;
        std::shared_ptr<service::service_provider> provider;
        thread_orchestrator::worker_handle worker;
    };
    std::vector<active_system> active_systems_;
};

}  // namespace corona::framework::runtime
