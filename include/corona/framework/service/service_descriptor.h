#pragma once

#include <functional>
#include <memory>
#include <typeindex>

#include "service_lifetime.h"

namespace corona::framework::service {

class service_provider;

class service_descriptor {
   public:
    using factory_type = std::function<std::shared_ptr<void>(service_provider&)>;

    service_descriptor(std::type_index service_type,
                       service_lifetime lifetime,
                       factory_type factory);

    [[nodiscard]] std::type_index service_type() const noexcept;
    [[nodiscard]] service_lifetime lifetime() const noexcept;
    [[nodiscard]] std::shared_ptr<void> create(service_provider& provider) const;

   private:
    std::type_index service_type_;
    service_lifetime lifetime_;
    factory_type factory_;
};

}  // namespace corona::framework::service
