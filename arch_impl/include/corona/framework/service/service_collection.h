#pragma once

#include "service_descriptor.h"
#include "service_lifetime.h"
#include "service_provider.h"

#include <functional>
#include <memory>
#include <typeindex>
#include <utility>
#include <vector>

namespace corona::framework::service {

class service_collection {
   public:
    service_collection() = default;
    ~service_collection() = default;

    service_collection(const service_collection&) = delete;
    service_collection& operator=(const service_collection&) = delete;
    service_collection(service_collection&&) noexcept = default;
    service_collection& operator=(service_collection&&) noexcept = default;

    template <typename TService, typename TImplementation>
    void add_singleton();

    template <typename TService>
    void add_singleton(std::shared_ptr<TService> instance);

    template <typename TService>
    void add_singleton(std::function<std::shared_ptr<TService>(service_provider&)> factory);

    template <typename TService, typename TImplementation>
    void add_scoped();

    template <typename TService>
    void add_scoped(std::function<std::shared_ptr<TService>(service_provider&)> factory);

    template <typename TService, typename TImplementation>
    void add_transient();

    template <typename TService>
    void add_transient(std::function<std::shared_ptr<TService>(service_provider&)> factory);

    [[nodiscard]] service_provider build_service_provider() const;

   private:
    void register_descriptor(std::type_index type,
                             service_lifetime lifetime,
                             service_descriptor::factory_type factory);

    std::vector<service_descriptor> descriptors_;
};

template <typename TService, typename TImplementation>
void service_collection::add_singleton() {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::singleton,
                        [](service_provider&) {
                            auto instance = std::make_shared<TImplementation>();
                            return std::shared_ptr<void>(instance);
                        });
}

template <typename TService>
void service_collection::add_singleton(std::shared_ptr<TService> instance) {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::singleton,
                        [instance = std::move(instance)](service_provider&) {
                            return std::shared_ptr<void>(instance);
                        });
}

template <typename TService>
void service_collection::add_singleton(std::function<std::shared_ptr<TService>(service_provider&)> factory) {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::singleton,
                        [factory = std::move(factory)](service_provider& provider) {
                            auto created = factory(provider);
                            return std::shared_ptr<void>(std::move(created));
                        });
}

template <typename TService, typename TImplementation>
void service_collection::add_scoped() {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::scoped,
                        [](service_provider&) {
                            auto instance = std::make_shared<TImplementation>();
                            return std::shared_ptr<void>(instance);
                        });
}

template <typename TService>
void service_collection::add_scoped(std::function<std::shared_ptr<TService>(service_provider&)> factory) {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::scoped,
                        [factory = std::move(factory)](service_provider& provider) {
                            auto created = factory(provider);
                            return std::shared_ptr<void>(std::move(created));
                        });
}

template <typename TService, typename TImplementation>
void service_collection::add_transient() {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::transient,
                        [](service_provider&) {
                            auto instance = std::make_shared<TImplementation>();
                            return std::shared_ptr<void>(instance);
                        });
}

template <typename TService>
void service_collection::add_transient(std::function<std::shared_ptr<TService>(service_provider&)> factory) {
    register_descriptor(std::type_index(typeid(TService)),
                        service_lifetime::transient,
                        [factory = std::move(factory)](service_provider& provider) {
                            auto created = factory(provider);
                            return std::shared_ptr<void>(std::move(created));
                        });
}

}  // namespace corona::framework::service
