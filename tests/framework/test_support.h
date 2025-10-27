#pragma once

#include <chrono>
#include <iostream>
#include <string_view>

class test_scope {
   public:
    explicit test_scope(std::string_view name)
        : name_(name), start_(std::chrono::steady_clock::now()) {
        std::cout << "[TEST][START] " << name_ << std::endl;
    }

    ~test_scope() {
        const auto finish = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start_);
        std::cout << "[TEST][END] " << name_ << " (" << elapsed.count() << " ms)" << std::endl;
    }

   private:
    std::string_view name_;
    std::chrono::steady_clock::time_point start_;
};

#ifndef CORONA_FRAMEWORK_TEST_ASSET_DIR
#define CORONA_FRAMEWORK_TEST_ASSET_DIR ""
#endif
