#include "corona/framework/service/service_provider.h"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace corona::framework::service {

service_scope::service_scope() = default;

std::shared_ptr<void> service_scope::get_scoped_instance(std::type_index type) {
    std::lock_guard<std::mutex> guard(scoped_mutex_);
    auto it = scoped_instances_.find(type);
    if (it == scoped_instances_.end()) {
        return nullptr;
    }
    return it->second;
}

void service_scope::set_scoped_instance(std::type_index type, std::shared_ptr<void> instance) {
    std::lock_guard<std::mutex> guard(scoped_mutex_);
    scoped_instances_[type] = std::move(instance);
}

std::shared_ptr<service_scope> service_scope::make_child() {
    auto child = std::make_shared<service_scope>();
    return child;
}

service_provider::service_provider(std::shared_ptr<const std::vector<service_descriptor>> descriptors,
                                   std::shared_ptr<service_scope> scope,
                                   std::shared_ptr<std::unordered_map<std::type_index, std::shared_ptr<void>>> singletons)
    : descriptors_(std::move(descriptors)),
      scope_(std::move(scope)),
      singletons_(std::move(singletons)) {
    if (!descriptors_) {
        descriptors_ = std::make_shared<const std::vector<service_descriptor>>();
    }
    if (!scope_) {
        scope_ = std::make_shared<service_scope>();
    }
    if (!singletons_) {
        singletons_ = std::make_shared<std::unordered_map<std::type_index, std::shared_ptr<void>>>();
    }
}

service_provider::service_provider(service_provider&& other) noexcept
    : descriptors_(std::move(other.descriptors_)),
      scope_(std::move(other.scope_)),
      singletons_(std::move(other.singletons_)) {}

service_provider& service_provider::operator=(service_provider&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(resolve_mutex_, other.resolve_mutex_);
        descriptors_ = std::move(other.descriptors_);
        scope_ = std::move(other.scope_);
        singletons_ = std::move(other.singletons_);
    }
    return *this;
}

service_provider service_provider::create_scope() {
    auto child_scope = scope_->make_child();
    return service_provider(descriptors_, child_scope, singletons_);
}

std::shared_ptr<void> service_provider::resolve(std::type_index type) {
    {
        std::lock_guard<std::mutex> guard(resolve_mutex_);
        auto singleton_it = singletons_->find(type);
        if (singleton_it != singletons_->end()) {
            return singleton_it->second;
        }
    }

    service_descriptor const* descriptor = nullptr;
    for (auto const& item : *descriptors_) {
        if (item.service_type() == type) {
            descriptor = &item;
            break;
        }
    }
    if (!descriptor) {
        throw std::runtime_error("service not registered");
    }

    std::shared_ptr<void> instance;
    switch (descriptor->lifetime()) {
        case service_lifetime::singleton: {
            std::lock_guard<std::mutex> guard(resolve_mutex_);
            auto singleton_it = singletons_->find(type);
            if (singleton_it != singletons_->end()) {
                return singleton_it->second;
            }
            instance = descriptor->create(*this);
            (*singletons_)[type] = instance;
            return instance;
        }
        case service_lifetime::scoped: {
            if (auto scoped = scope_->get_scoped_instance(type)) {
                return scoped;
            }
            instance = descriptor->create(*this);
            scope_->set_scoped_instance(type, instance);
            return instance;
        }
        case service_lifetime::transient:
            return descriptor->create(*this);
        default:
            throw std::runtime_error("unknown service lifetime");
    }
}

}  // namespace corona::framework::service
