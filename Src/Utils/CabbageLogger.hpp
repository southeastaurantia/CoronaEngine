//
// Created by 47226 on 2025/9/3.
//

#ifndef CABBAGEFRAMEWORK_UTILS_H
#define CABBAGEFRAMEWORK_UTILS_H

#include <format>
#include <iostream>

#define LOG_DEBUG(message)       \
    if constexpr (LOG_LEVEL < 1) \
    std::cout << std::format("[DEBUG][CabbageFW] {}", message) << std::endl
#define LOG_INFO(message)        \
    if constexpr (LOG_LEVEL < 2) \
    std::cout << std::format("[INFO ][CabbageFW] {}", message) << std::endl
#define LOG_WARNING(message)     \
    if constexpr (LOG_LEVEL < 3) \
    std::cout << std::format("[WARN ][CabbageFW] {}", message) << std::endl
#define LOG_ERROR(message)       \
    if constexpr (LOG_LEVEL < 4) \
    std::cerr << std::format("[ERROR][CabbageFW] {}", message) << std::endl

#endif // CABBAGEFRAMEWORK_UTILS_H
