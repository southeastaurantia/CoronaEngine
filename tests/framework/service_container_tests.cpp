#include <cassert>
#include <memory>
#include <string>
#include <utility>

#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"
#include "test_registry.h"
#include "test_support.h"

namespace {

struct base_service {
    virtual ~base_service() = default;
    virtual std::string name() const = 0;
};

struct derived_service final : base_service {
    std::string payload;

    explicit derived_service(std::string value = "default")
        : payload(std::move(value)) {}

    std::string name() const override {
        return payload;
    }
};

struct dependent_service {
    std::shared_ptr<base_service> base;

    explicit dependent_service(std::shared_ptr<base_service> input)
        : base(std::move(input)) {}
};

}  // namespace

void service_container_tests() {
    const test_scope test_marker("service_container_tests");
    corona::framework::service::service_collection collection;
    collection.add_singleton<base_service, derived_service>();
    collection.add_scoped<dependent_service>([](corona::framework::service::service_provider& provider) {
        return std::make_shared<dependent_service>(provider.get_service<base_service>());
    });
    collection.add_transient<std::string>([](corona::framework::service::service_provider&) {
        return std::make_shared<std::string>("transient");
    });

    auto provider = collection.build_service_provider();
    auto singleton_a = provider.get_service<base_service>();
    auto singleton_b = provider.get_service<base_service>();
    assert(singleton_a == singleton_b);
    assert(singleton_a->name() == "default");

    auto scoped_provider = provider.create_scope();
    auto scoped_a = scoped_provider.get_service<dependent_service>();
    auto scoped_b = scoped_provider.get_service<dependent_service>();
    assert(scoped_a == scoped_b);
    assert(scoped_a->base == singleton_a);

    auto other_scope = provider.create_scope();
    auto scoped_c = other_scope.get_service<dependent_service>();
    assert(scoped_c != scoped_a);

    auto transient_a = provider.get_service<std::string>();
    auto transient_b = provider.get_service<std::string>();
    assert(transient_a != transient_b);
    assert(*transient_a == "transient");
}
