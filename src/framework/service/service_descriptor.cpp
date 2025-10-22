#include "corona/framework/service/service_descriptor.h"

#include <stdexcept>
#include <utility>

#include "corona/framework/service/service_provider.h"

namespace corona::framework::service {

service_descriptor::service_descriptor(std::type_index service_type,
                                       service_lifetime lifetime,
                                       factory_type factory)
    : service_type_(service_type), lifetime_(lifetime), factory_(std::move(factory)) {
    if (!factory_) {
        throw std::invalid_argument("service_descriptor requires non-null factory");
    }
}

std::type_index service_descriptor::service_type() const noexcept {
    return service_type_;
}

service_lifetime service_descriptor::lifetime() const noexcept {
    return lifetime_;
}

std::shared_ptr<void> service_descriptor::create(service_provider& provider) const {
    return factory_(provider);
}

}  // namespace corona::framework::service
