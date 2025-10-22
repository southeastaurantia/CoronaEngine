#include "corona/framework/service/service_collection.h"

#include "corona/framework/service/service_descriptor.h"
#include "corona/framework/service/service_provider.h"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace corona::framework::service {

void service_collection::register_descriptor(std::type_index type,
                                             service_lifetime lifetime,
                                             service_descriptor::factory_type factory) {
    descriptors_.emplace_back(type, lifetime, std::move(factory));
}

service_provider service_collection::build_service_provider() const {
    auto copied = std::make_shared<std::vector<service_descriptor>>(descriptors_);
    auto singleton_map = std::make_shared<std::unordered_map<std::type_index, std::shared_ptr<void>>>();
    return service_provider(std::move(copied), std::make_shared<service_scope>(), std::move(singleton_map));
}

}  // namespace corona::framework::service
