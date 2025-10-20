#pragma once

#include <corona/core/Engine.h>
#include <corona/core/SystemRegistry.h>
#include <corona/interfaces/SystemContext.h>
#include <corona/interfaces/ThreadedSystem.h>
#include <corona_logger.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Example {
struct DiagnosticsConfig {
    std::vector<std::string> requested_systems{};
    std::uint32_t log_interval_ms = 750;  // default heartbeat once every ~0.75s
};

class DiagnosticsService {
   public:
    explicit DiagnosticsService(Corona::Engine& engine)
        : engine_(&engine) {
    }

    void set_requested(std::vector<std::string> systems) {
        requested_systems_ = std::move(systems);
    }

    void log_heartbeat(std::string_view source, std::uint64_t heartbeat) const {
        CE_LOG_INFO("[diag] {} heartbeat {} | requested: {} | registry: {}",
                    source,
                    heartbeat,
                    join(requested_systems_),
                    collect_registered());
    }

   private:
    Corona::Engine* engine_;
    std::vector<std::string> requested_systems_;

    static std::string join(const std::vector<std::string>& values) {
        if (values.empty()) {
            return std::string{"(none)"};
        }
        std::ostringstream stream;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                stream << ", ";
            }
            stream << values[i];
        }
        return stream.str();
    }

    std::string collect_registered() const {
        if (!engine_) {
            return std::string{"(engine unavailable)"};
        }
        auto descriptors = engine_->system_registry().list();
        if (descriptors.empty()) {
            return std::string{"(none)"};
        }
        std::ostringstream stream;
        for (size_t i = 0; i < descriptors.size(); ++i) {
            if (i > 0) {
                stream << ", ";
            }
            stream << descriptors[i]->name;
        }
        return stream.str();
    }
};

class DiagnosticsSystem final : public Corona::ThreadedSystem {
   public:
    static constexpr const char* kName = "DiagnosticsSystem";

    explicit DiagnosticsSystem(std::shared_ptr<DiagnosticsConfig> config)
        : Corona::ThreadedSystem(kName, 30),  // run at ~30 FPS for periodic sampling
          config_(std::move(config)) {
    }

   protected:
    void onStart() override {
        if (config_ && config_->log_interval_ms > 0U) {
            interval_ = std::chrono::milliseconds{config_->log_interval_ms};
        }
        service_ = services().try_get<DiagnosticsService>();
        last_log_ = std::chrono::steady_clock::now();
    }

    void onTick() override {
        if (!service_) {
            service_ = services().try_get<DiagnosticsService>();
            if (!service_) {
                return;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - last_log_ < interval_) {
            return;
        }
        last_log_ = now;
        service_->log_heartbeat(kName, ++heartbeat_);
    }

   private:
    std::shared_ptr<DiagnosticsConfig> config_;
    std::shared_ptr<DiagnosticsService> service_;
    std::chrono::steady_clock::time_point last_log_{};
    std::chrono::milliseconds interval_{750};
    std::uint64_t heartbeat_ = 0;
};

inline std::string JoinNames(const std::vector<std::string>& values) {
    if (values.empty()) {
        return std::string{};
    }
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            stream << ",";
        }
        stream << values[i];
    }
    return stream.str();
}

inline void RegisterDiagnosticsPlugin(Corona::SystemRegistry& registry) {
    if (registry.contains(DiagnosticsSystem::kName)) {
        return;
    }

    registry.register_plugin({.name = DiagnosticsSystem::kName,
                              .dependencies = {"RenderingSystem"},
                              .factory = [](const Corona::Interfaces::SystemContext& context) {
                                  auto config = context.services.try_get<DiagnosticsConfig>();
                                  if (!config) {
                                      config = std::make_shared<DiagnosticsConfig>();
                                  }
                                  return std::make_shared<DiagnosticsSystem>(std::move(config));
                              },
                              .description = "Periodically logs system state using injected services"});
}

}  // namespace Example
