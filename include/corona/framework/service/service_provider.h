#pragma once

#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "service_descriptor.h"

namespace corona::framework::service {

class service_scope;

class service_provider {
   public:
    service_provider(std::shared_ptr<const std::vector<service_descriptor>> descriptors,
                     std::shared_ptr<service_scope> scope,
                     std::shared_ptr<std::unordered_map<std::type_index, std::shared_ptr<void>>> singletons);

    service_provider(service_provider&& other) noexcept;
    service_provider& operator=(service_provider&& other) noexcept;
    service_provider(const service_provider&) = delete;
    service_provider& operator=(const service_provider&) = delete;

    template <typename T>
    [[nodiscard]] std::shared_ptr<T> get_service();

    [[nodiscard]] service_provider create_scope();

   private:
    friend class service_scope;

    std::shared_ptr<void> resolve(std::type_index type);

    std::shared_ptr<const std::vector<service_descriptor>> descriptors_;
    std::shared_ptr<service_scope> scope_;
    std::shared_ptr<std::unordered_map<std::type_index, std::shared_ptr<void>>> singletons_;
    std::mutex resolve_mutex_;
};

class service_scope : public std::enable_shared_from_this<service_scope> {
   public:
    service_scope();

    std::shared_ptr<void> get_scoped_instance(std::type_index type);
    void set_scoped_instance(std::type_index type, std::shared_ptr<void> instance);

    [[nodiscard]] std::shared_ptr<service_scope> make_child();

   private:
    std::mutex scoped_mutex_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> scoped_instances_;
};

template <typename T>
std::shared_ptr<T> service_provider::get_service() {
    auto raw = resolve(std::type_index(typeid(T)));
    return std::static_pointer_cast<T>(raw);
}

}  // namespace corona::framework::service
