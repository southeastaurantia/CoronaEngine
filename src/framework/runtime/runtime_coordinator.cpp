#include "corona/framework/runtime/runtime_coordinator.h"

#include <functional>
#include <memory>
#include <stdexcept>

namespace corona::framework::runtime {

runtime_coordinator::runtime_coordinator() = default;

void runtime_coordinator::configure_services(service::service_collection collection) {
    ensure_not_running();
    service_collection_ = std::make_unique<service::service_collection>(std::move(collection));
    root_provider_.reset();
}

void runtime_coordinator::register_descriptor(system_descriptor descriptor) {
    ensure_not_running();
    if (descriptor.id.empty()) {
        throw std::invalid_argument("system descriptor requires id");
    }
    if (!descriptor.factory) {
        throw std::invalid_argument("system descriptor requires factory");
    }
    auto [it, inserted] = descriptors_.emplace(descriptor.id, descriptor);
    if (!inserted) {
        throw std::invalid_argument("duplicate system descriptor id");
    }
    initialized_ = false;
}

void runtime_coordinator::register_factory(std::string name, std::shared_ptr<system_factory> factory) {
    ensure_not_running();
    if (name.empty()) {
        throw std::invalid_argument("factory name cannot be empty");
    }
    if (!factory) {
        throw std::invalid_argument("factory must be valid");
    }
    auto [it, inserted] = factories_.emplace(std::move(name), std::move(factory));
    if (!inserted) {
        throw std::invalid_argument("duplicate factory registration");
    }
}

void runtime_coordinator::register_manifest(plugin::plugin_manifest manifest) {
    ensure_not_running();
    manifests_.push_back(std::move(manifest));
    initialized_ = false;
}

void runtime_coordinator::load_manifests(const configuration& config) {
    ensure_not_running();
    for (auto const& path : config.manifest_paths) {
        manifests_.push_back(plugin::load_manifest(path));
    }
    initialized_ = false;
}

void runtime_coordinator::initialize() {
    ensure_not_running();
    hydrate_manifests();
    if (!service_collection_) {
        service_collection_ = std::make_unique<service::service_collection>();
    }
    if (!root_provider_) {
        root_provider_.emplace(service_collection_->build_service_provider());
    }
    resolve_dependency_order();
    initialized_ = true;
}

void runtime_coordinator::start() {
    if (running_) {
        return;
    }
    if (!initialized_) {
        initialize();
    }
    active_systems_.clear();
    active_systems_.reserve(startup_order_.size());

    for (auto const& id : startup_order_) {
        auto const& descriptor = descriptor_for(id);
        auto instance = descriptor.factory->create();
        if (!instance) {
            throw std::runtime_error("system factory returned null instance");
        }
        auto provider = std::make_shared<service::service_provider>(root_provider_->create_scope());

        active_systems_.emplace_back();
        auto& slot = active_systems_.back();
        slot.id = descriptor.id;
        slot.instance = std::move(instance);
        slot.provider = std::move(provider);

        system_context context{*slot.provider, messaging_hub_, orchestrator_};
        slot.instance->configure(context);
        slot.instance->start();

        auto task = [system_ptr = slot.instance.get()](worker_control& control) {
            system_ptr->execute(control);
        };
        thread_orchestrator::worker_options worker_options{};
        worker_options.tick_interval = descriptor.tick_interval;
        slot.worker = orchestrator_.add_worker(descriptor.id, worker_options, std::move(task));
    }

    running_ = true;
}

void runtime_coordinator::stop() {
    if (!running_) {
        return;
    }
    for (auto& active : active_systems_) {
        active.worker.stop();
        if (active.instance) {
            active.instance->stop();
        }
    }
    active_systems_.clear();
    orchestrator_.stop_all();
    running_ = false;
}

void runtime_coordinator::ensure_not_running() const {
    if (running_) {
        throw std::logic_error("operation not allowed while runtime is running");
    }
}

void runtime_coordinator::hydrate_manifests() {
    for (auto const& manifest : manifests_) {
        for (auto const& system : manifest.systems) {
            if (descriptors_.find(system.id) != descriptors_.end()) {
                continue;
            }
            auto factory_it = factories_.find(system.factory);
            if (factory_it == factories_.end()) {
                throw std::runtime_error("missing factory for system: " + system.factory);
            }
            system_descriptor descriptor(system.id, factory_it->second);
            descriptor.display_name = system.id;
            descriptor.dependencies = system.dependencies;
            descriptor.tags = system.tags;
            descriptor.tick_interval = system.tick_interval;
            descriptors_.emplace(descriptor.id, std::move(descriptor));
        }
    }
}

void runtime_coordinator::resolve_dependency_order() {
    enum class visit_state { none,
                             visiting,
                             done };
    std::unordered_map<std::string, visit_state> states;
    std::vector<std::string> order;
    order.reserve(descriptors_.size());

    std::function<void(const std::string&)> visit = [&](const std::string& id) {
        auto state = states[id];
        if (state == visit_state::done) {
            return;
        }
        if (state == visit_state::visiting) {
            throw std::runtime_error("system dependency cycle detected at " + id);
        }
        states[id] = visit_state::visiting;
        auto const& desc = descriptor_for(id);
        for (auto const& dep : desc.dependencies) {
            if (descriptors_.find(dep) == descriptors_.end()) {
                throw std::runtime_error("missing dependency descriptor: " + dep);
            }
            visit(dep);
        }
        states[id] = visit_state::done;
        order.push_back(id);
    };

    for (auto const& [id, _] : descriptors_) {
        visit(id);
    }

    startup_order_ = std::move(order);
}

const system_descriptor& runtime_coordinator::descriptor_for(const std::string& id) const {
    auto it = descriptors_.find(id);
    if (it == descriptors_.end()) {
        throw std::runtime_error("unknown system descriptor: " + id);
    }
    return it->second;
}

}  // namespace corona::framework::runtime
