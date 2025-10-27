#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <thread>

#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"
#include "corona/framework/services/logging/logging_setup.h"
#include "test_registry.h"
#include "test_support.h"

namespace logging = corona::framework::services::logging;

void logging_service_tests() {
    const test_scope test_marker("logging_service_tests");
    corona::framework::service::service_collection collection;
    auto temp_log = std::filesystem::temp_directory_path() / "corona_framework_logging_test.log";
    std::error_code remove_ec;
    std::filesystem::remove(temp_log, remove_ec);

    logging::logging_config config;
    config.enable_console = true;
    config.console_level = logging::log_level::trace;
    config.enable_file = true;
    config.file_path = temp_log;
    config.file_level = logging::log_level::trace;
    config.file_append = false;
    config.file_flush_on_log = true;

    std::cout << "[logging_service_tests][config] file=" << temp_log << std::endl;
    auto registered = logging::register_logging_services(collection, config);
    assert(registered);

    auto provider = collection.build_service_provider();
    auto logger = provider.get_service<logging::logger>();
    assert(logger);

    assert(logger == registered);

    std::cout << "[logging_service_tests][run] writing probe entries" << std::endl;
    logger->info("logging smoke");
    logger->info(temp_log.string());

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool found = false;
    std::cout << "[logging_service_tests][verify] polling for file output" << std::endl;
    while (!found && std::chrono::steady_clock::now() < deadline) {
        if (std::filesystem::exists(temp_log)) {
            std::ifstream in(temp_log);
            std::string line;
            while (std::getline(in, line)) {
                if (line.find("logging smoke") != std::string::npos) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    assert(found);

    std::filesystem::remove(temp_log, remove_ec);

    auto logger_again = provider.get_service<logging::logger>();
    assert(logger == logger_again);
}
