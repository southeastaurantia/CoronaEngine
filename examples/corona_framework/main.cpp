#include "corona/framework/messaging/event_stream.h"
#include "corona/framework/messaging/messaging_hub.h"
#include "corona/framework/plugin/plugin_manifest.h"
#include "corona/framework/runtime/runtime_coordinator.h"
#include "corona/framework/runtime/system.h"
#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace cfw = corona::framework;

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
    }

    void start() override {
        std::cout << "[beta.system] ready on thread " << std::this_thread::get_id() << std::endl;
    }

    void execute(cfw::runtime::worker_control& control) override {
        if (!metrics_ || !stream_) {
            control.request_stop();
            return;
        }

        auto current = ++metrics_->published;
        stream_->publish(static_cast<int>(current));
        std::cout << "[beta.system] published " << current << " on thread " << std::this_thread::get_id() << std::endl;

        if (current >= target_count_) {
            std::cout << "[beta.system] reached target" << std::endl;
            control.request_stop();
        }

        control.sleep_for(std::chrono::milliseconds(50));
    }

    void stop() override {
        std::cout << "[beta.system] stopped" << std::endl;
    }

   private:
    static constexpr int target_count_ = 5;

    std::shared_ptr<metrics_state> metrics_;
    std::shared_ptr<cfw::messaging::event_stream<int>> stream_;
};

class alpha_system final : public cfw::runtime::system {
   public:
    std::string_view id() const noexcept override {
        return "alpha.system";
    }

    void configure(const cfw::runtime::system_context& context) override {
        metrics_ = context.services.get_service<metrics_state>();
        stream_ = context.messaging.acquire_event_stream<int>("demo.counter");
    }

    void start() override {
        if (stream_) {
            subscription_ = stream_->subscribe();
            std::cout << "[alpha.system] subscribed to demo.counter on thread " << std::this_thread::get_id()
                      << std::endl;
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

    std::cout << "[alpha.system] observed " << value << " on thread " << std::this_thread::get_id() << std::endl;
        if (value >= stop_threshold_) {
            std::cout << "[alpha.system] threshold met; shutting down" << std::endl;
            control.request_stop();
        }
    }

    void stop() override {
        subscription_.close();
        if (auto metrics = metrics_) {
            std::cout << "[alpha.system] total messages observed: " << metrics->published.load() << std::endl;
        }
        std::cout << "[alpha.system] stopped" << std::endl;
    }

   private:
    static constexpr int stop_threshold_ = 5;

    std::shared_ptr<metrics_state> metrics_;
    std::shared_ptr<cfw::messaging::event_stream<int>> stream_;
    cfw::messaging::event_subscription<int> subscription_;
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

        std::cout << "Example completed; total events published: " << metrics->published.load() << std::endl;
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "corona_framework_example failed: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
}
