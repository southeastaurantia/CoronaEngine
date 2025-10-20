#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>

namespace Corona::Interfaces {

class ServiceLocator {
  public:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;
    ServiceLocator(ServiceLocator&&) = delete;
    ServiceLocator& operator=(ServiceLocator&&) = delete;

    template <typename TService>
    void register_service(std::shared_ptr<TService> service) {
        if (!service) {
            return;
        }
        const std::type_index key{typeid(TService)};
        {
            std::unique_lock lock(mutex_);
            services_[key] = std::move(service);
        }
    }

    template <typename TService>
    [[nodiscard]] std::shared_ptr<TService> try_get() const {
        const std::type_index key{typeid(TService)};
        std::shared_lock lock(mutex_);
        if (auto it = services_.find(key); it != services_.end()) {
            return std::static_pointer_cast<TService>(it->second);
        }
        return nullptr;
    }

    template <typename TService>
    [[nodiscard]] TService& require() const {
        auto ptr = try_get<TService>();
        if (!ptr) {
            throw std::runtime_error("Service not registered");
        }
        return *ptr;
    }

    template <typename TService>
    [[nodiscard]] bool contains() const {
        const std::type_index key{typeid(TService)};
        std::shared_lock lock(mutex_);
        return services_.contains(key);
    }

  private:
    mutable std::shared_mutex mutex_{};
    std::unordered_map<std::type_index, std::shared_ptr<void>> services_{};
};

} // namespace Corona::Interfaces
