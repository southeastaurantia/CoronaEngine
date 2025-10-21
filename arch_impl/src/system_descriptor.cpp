#include "corona/framework/runtime/system.h"

#include <stdexcept>
#include <utility>

namespace corona::framework::runtime {

system_descriptor::system_descriptor(std::string system_id, std::shared_ptr<system_factory> system_factory_ptr)
    : id(std::move(system_id)), display_name(id), factory(std::move(system_factory_ptr)) {
    if (id.empty()) {
        throw std::invalid_argument("system_descriptor requires non-empty id");
    }
    if (!factory) {
        throw std::invalid_argument("system_descriptor requires valid factory");
    }
}

}  // namespace corona::framework::runtime
