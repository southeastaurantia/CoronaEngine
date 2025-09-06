//
// Created by 47226 on 2025/9/3.
//

#ifndef CABBAGEFRAMEWORK_UTILS_H
#define CABBAGEFRAMEWORK_UTILS_H

#include <chrono>
#include <format>
#include <iostream>

#define LOG_DEBUG(message)       \
    if constexpr (LOG_LEVEL < 1) \
    std::cout << std::format("[{0:%F}T{0:%T}][DEBUG][CoronaEngine] {1}", std::chrono::system_clock::now(), message) << std::endl
#define LOG_INFO(message)        \
    if constexpr (LOG_LEVEL < 2) \
    std::cout << std::format("[{0:%F}T{0:%T}][INFO ][CoronaEngine] {1}", std::chrono::system_clock::now(), message) << std::endl
#define LOG_WARNING(message)     \
    if constexpr (LOG_LEVEL < 3) \
    std::cout << std::format("[{0:%F}T{0:%T}][WARN ][CoronaEngine] {1}", std::chrono::system_clock::now(), message) << std::endl
#define LOG_ERROR(message)       \
    if constexpr (LOG_LEVEL < 4) \
    std::cerr << std::format("[{0:%F}T{0:%T}][ERROR][CoronaEngine] {1}", std::chrono::system_clock::now(), message) << std::endl

#endif // CABBAGEFRAMEWORK_UTILS_H
