#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <thread>

#include "corona/framework/messaging/event_stream.h"
#include "corona/framework/messaging/messaging_hub.h"
#include "corona/framework/runtime/runtime_coordinator.h"
#include "corona/framework/runtime/system.h"
#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"
#include "corona/framework/services/logging/console_logger.h"
#include "corona/framework/services/logging/logger.h"

namespace cfw = corona::framework;
namespace logging = corona::framework::services::logging;

struct metrics_state {
    std::atomic<int> published{0};
};

class beta_system final : public cfw::runtime::system {
   public:
    std::string_view id() const noexcept override {
        return "beta.system";
    }

    void configure(const cfw::runtime::system_context& context) override {
        metrics_ = context.services.get_service<metrics_state>();
        stream_ = context.messaging.acquire_event_stream<int>("demo.counter");
        logger_ = context.services.get_service<logging::logger>();
    }

    void start() override {
        if (logger_) {
            logger_->info("[beta.system] ready");
        }
    }

    void execute(cfw::runtime::worker_control& control) override {
        if (!metrics_ || !stream_) {
            control.request_stop();
            return;
        }

        auto current = ++metrics_->published;
        stream_->publish(static_cast<int>(current));
        if (logger_) {
            std::ostringstream oss;
            oss << "[beta.system] published " << current;
            auto message = oss.str();
            logger_->info(message);
        }

        if (current >= target_count_) {
            if (logger_) {
                logger_->info("[beta.system] reached target");
            }
            control.request_stop();
        }

        control.sleep_for(std::chrono::milliseconds(50));
    }

    void stop() override {
        if (logger_) {
            logger_->info("[beta.system] stopped");
        }
    }

   private:
    static constexpr int target_count_ = 5;

    std::shared_ptr<metrics_state> metrics_;
    std::shared_ptr<cfw::messaging::event_stream<int>> stream_;
    std::shared_ptr<logging::logger> logger_;
};

class alpha_system final : public cfw::runtime::system {
   public:
    std::string_view id() const noexcept override {
        return "alpha.system";
    }

    void configure(const cfw::runtime::system_context& context) override {
        metrics_ = context.services.get_service<metrics_state>();
        stream_ = context.messaging.acquire_event_stream<int>("demo.counter");
        logger_ = context.services.get_service<logging::logger>();
    }

    void start() override {
        if (stream_) {
            subscription_ = stream_->subscribe();
            if (logger_) {
                logger_->info("[alpha.system] subscribed to demo.counter");
            }
        }
    }

    void execute(cfw::runtime::worker_control& control) override {
        if (!subscription_.valid()) {
            control.request_stop();
            return;
        }

        int value = 0;
        if (!subscription_.wait_for(value, std::chrono::milliseconds(100))) {
            return;
        }

        if (logger_) {
            std::ostringstream oss;
            oss << "[alpha.system] observed " << value;
            auto message = oss.str();
            logger_->info(message);
        }
        if (value >= stop_threshold_) {
            if (logger_) {
                logger_->info("[alpha.system] threshold met; shutting down");
            }
            control.request_stop();
        }
    }

    void stop() override {
        subscription_.close();
        if (auto metrics = metrics_) {
            if (logger_) {
                std::ostringstream oss;
                oss << "[alpha.system] total messages observed: " << metrics->published.load();
                auto message = oss.str();
                logger_->info(message);
            }
        }
        if (logger_) {
            logger_->info("[alpha.system] stopped");
        }
    }

   private:
    static constexpr int stop_threshold_ = 5;

    std::shared_ptr<metrics_state> metrics_;
    std::shared_ptr<cfw::messaging::event_stream<int>> stream_;
    cfw::messaging::event_subscription<int> subscription_;
    std::shared_ptr<logging::logger> logger_;
};

int main(int argc, char** argv) {
    try {
        namespace fs = std::filesystem;

        fs::path manifest_path;
        if (argc > 0 && argv && argv[0]) {
            std::error_code ec;
            auto exe_path = fs::weakly_canonical(fs::absolute(fs::path(argv[0])), ec);
            if (ec) {
                exe_path = fs::absolute(fs::path(argv[0]));
            }
            manifest_path = exe_path.parent_path() / "sample_manifest.json";
        }

        if (manifest_path.empty() || !fs::exists(manifest_path)) {
            manifest_path = fs::current_path() / "sample_manifest.json";
        }

        std::error_code ec;
        manifest_path = fs::weakly_canonical(manifest_path, ec);
        if (ec) {
            manifest_path = fs::absolute(manifest_path);
        }

        if (!fs::exists(manifest_path)) {
            throw std::runtime_error("sample_manifest.json not found next to executable");
        }

        auto metrics = std::make_shared<metrics_state>();

        cfw::runtime::runtime_coordinator coordinator;

        cfw::service::service_collection services;
        auto example_logger = logging::register_console_logger(services, logging::log_level::info);
        services.add_singleton<metrics_state>(metrics);
        coordinator.configure_services(std::move(services));

        coordinator.register_factory("beta.factory", std::make_shared<cfw::runtime::default_system_factory<beta_system>>());
        coordinator.register_factory("alpha.factory", std::make_shared<cfw::runtime::default_system_factory<alpha_system>>());

        cfw::runtime::runtime_coordinator::configuration config;
        config.manifest_paths.push_back(manifest_path);
        coordinator.load_manifests(config);

        coordinator.initialize();
        coordinator.start();

        std::this_thread::sleep_for(std::chrono::seconds(1));
        coordinator.stop();

        if (example_logger) {
            std::ostringstream oss;
            oss << "[example] completed; total events published: " << metrics->published.load();
            auto message = oss.str();
            example_logger->info(message);
        }
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "corona_framework_example failed: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
