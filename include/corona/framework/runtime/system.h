#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace corona::framework::service {
class service_provider;
}

namespace corona::framework::messaging {
class messaging_hub;
}

namespace corona::framework::services::time {
class time_service;
}

namespace corona::framework::runtime {

class thread_orchestrator;
class worker_control;

struct system_context {
    service::service_provider& services;
    messaging::messaging_hub& messaging;
    thread_orchestrator& orchestrator;
    corona::framework::services::time::time_service& time;
};

class system {
   public:
    virtual ~system() = default;
    virtual std::string_view id() const noexcept = 0;
    virtual void configure(const system_context& context) = 0;
    virtual void start() = 0;
    virtual void execute(worker_control& control) = 0;
    virtual void stop() = 0;
};

class system_factory {
   public:
    virtual ~system_factory() = default;
    virtual std::unique_ptr<system> create() = 0;
};

template <typename TSystem>
class default_system_factory final : public system_factory {
   public:
    std::unique_ptr<system> create() override {
        return std::make_unique<TSystem>();
    }
};

struct system_descriptor {
    using milliseconds = std::chrono::milliseconds;

    std::string id;
    std::string display_name;
    std::vector<std::string> dependencies;
    std::vector<std::string> tags;
    milliseconds tick_interval{16};
    std::shared_ptr<system_factory> factory;

    system_descriptor() = default;
    system_descriptor(std::string system_id, std::shared_ptr<system_factory> system_factory_ptr);
};

}  // namespace corona::framework::runtime
