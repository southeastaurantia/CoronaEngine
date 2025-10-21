#pragma once

namespace corona::framework::service {

enum class service_lifetime {
    singleton,
    scoped,
    transient
};

}  // namespace corona::framework::service
