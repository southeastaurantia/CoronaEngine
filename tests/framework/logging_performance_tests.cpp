#include "test_registry.h"
#include "test_support.h"

#include <array>
#include <cassert>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <thread>

#include "corona/framework/service/service_collection.h"
#include "corona/framework/service/service_provider.h"
#include "corona/framework/services/logging/logging_setup.h"

namespace logging = corona::framework::services::logging;

void logging_performance_tests() {
    const test_scope test_marker("logging_performance_tests");
    corona::framework::service::service_collection collection;
    auto temp_log = std::filesystem::temp_directory_path() / "corona_framework_logging_perf.log";
    std::error_code remove_ec;
    std::filesystem::remove(temp_log, remove_ec);

    logging::logging_config config;
    config.enable_console = false;
    config.enable_file = true;
    config.file_path = temp_log;
    config.file_level = logging::log_level::info;
    config.file_append = false;
    config.file_flush_on_log = false;
    config.queue_capacity = 32768;

    std::cout << "[logging_performance_tests][config] file=" << temp_log
              << ", queue_capacity=" << config.queue_capacity << std::endl;
    auto registered = logging::register_logging_services(collection, config);
    assert(registered);

    auto provider = collection.build_service_provider();
    auto logger = provider.get_service<logging::logger>();
    assert(logger);

    constexpr std::size_t iterations = 50000;
    constexpr std::string_view prefix = "logging perf ";
    std::array<char, 64> buffer{};

    std::cout << "[logging_performance_tests][run] iterations=" << iterations << std::endl;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        std::memcpy(buffer.data(), prefix.data(), prefix.size());
        auto [ptr, ec] = std::to_chars(buffer.data() + prefix.size(), buffer.data() + buffer.size(), i);
        assert(ec == std::errc{});
        const auto length = prefix.size() + static_cast<std::size_t>(ptr - (buffer.data() + prefix.size()));
        logger->info(std::string_view(buffer.data(), length));
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "logging_performance_tests elapsed: " << elapsed.count() << " ms" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    const auto max_allowed = std::chrono::milliseconds(3000);
    assert(elapsed < max_allowed);

    assert(std::filesystem::exists(temp_log));
    const auto file_size = std::filesystem::file_size(temp_log);
    std::cout << "[logging_performance_tests][verify] file_size=" << file_size << " bytes" << std::endl;
    assert(file_size > 0);

    logger.reset();
    registered.reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::filesystem::remove(temp_log, remove_ec);
}
