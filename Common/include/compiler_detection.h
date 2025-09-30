#pragma once

/**
 * @file compiler_detection.h
 * @brief CoronaEngine 全局编译器、平台和架构检测宏定义
 * 
 * 此文件为整个 CoronaEngine 项目提供统一的编译器检测宏定义。
 * 这些宏在 CMake 配置时通过 CoronaCompileConfig.cmake 注入。
 * 
 * @note 此文件为全局头文件，可以被项目中的任何模块包含和使用。
 */

// ========================================
// 编译器检测宏 (Compiler Detection)
// ========================================

#ifdef CORONA_COMPILER_MSVC
    #define CORONA_MSVC 1
    #define CORONA_FORCE_INLINE __forceinline
    #define CORONA_NEVER_INLINE __declspec(noinline)
    #define CORONA_PACK_PUSH(n) __pragma(pack(push, n))
    #define CORONA_PACK_POP() __pragma(pack(pop))
    #define CORONA_PACKED_STRUCT(name) CORONA_PACK_PUSH(1) struct name
    #define CORONA_PACKED_END CORONA_PACK_POP()
    #define CORONA_FUNCTION_SIGNATURE __FUNCSIG__
    #define CORONA_UNREACHABLE() __assume(0)
    #define CORONA_LIKELY(x) (x)
    #define CORONA_UNLIKELY(x) (x)
    #define CORONA_RESTRICT __restrict
    #define CORONA_THREAD_LOCAL __declspec(thread)
#else
    #define CORONA_MSVC 0
#endif

#ifdef CORONA_COMPILER_GCC
    #define CORONA_GCC 1
    #define CORONA_FORCE_INLINE __attribute__((always_inline)) inline
    #define CORONA_NEVER_INLINE __attribute__((noinline))
    #define CORONA_PACKED_STRUCT(name) struct __attribute__((packed)) name
    #define CORONA_PACKED_END
    #define CORONA_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
    #define CORONA_UNREACHABLE() __builtin_unreachable()
    #define CORONA_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CORONA_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define CORONA_RESTRICT __restrict__
    #define CORONA_THREAD_LOCAL __thread
#else
    #define CORONA_GCC 0
#endif

#ifdef CORONA_COMPILER_CLANG
    #define CORONA_CLANG 1
    #define CORONA_FORCE_INLINE __attribute__((always_inline)) inline
    #define CORONA_NEVER_INLINE __attribute__((noinline))
    #define CORONA_PACKED_STRUCT(name) struct __attribute__((packed)) name
    #define CORONA_PACKED_END
    #define CORONA_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
    #define CORONA_UNREACHABLE() __builtin_unreachable()
    #define CORONA_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CORONA_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define CORONA_RESTRICT __restrict__
    #define CORONA_THREAD_LOCAL __thread
#else
    #define CORONA_CLANG 0
#endif

#ifdef CORONA_COMPILER_APPLE_CLANG
    #define CORONA_APPLE_CLANG 1
    // Apple Clang 使用与标准 Clang 相同的属性
    #ifndef CORONA_FORCE_INLINE
        #define CORONA_FORCE_INLINE __attribute__((always_inline)) inline
    #endif
    #ifndef CORONA_NEVER_INLINE
        #define CORONA_NEVER_INLINE __attribute__((noinline))
    #endif
    #ifndef CORONA_PACKED_STRUCT
        #define CORONA_PACKED_STRUCT(name) struct __attribute__((packed)) name
        #define CORONA_PACKED_END
    #endif
    #ifndef CORONA_FUNCTION_SIGNATURE
        #define CORONA_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
    #endif
    #ifndef CORONA_UNREACHABLE
        #define CORONA_UNREACHABLE() __builtin_unreachable()
    #endif
    #ifndef CORONA_LIKELY
        #define CORONA_LIKELY(x) __builtin_expect(!!(x), 1)
        #define CORONA_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #endif
    #ifndef CORONA_RESTRICT
        #define CORONA_RESTRICT __restrict__
    #endif
    #ifndef CORONA_THREAD_LOCAL
        #define CORONA_THREAD_LOCAL __thread
    #endif
#else
    #define CORONA_APPLE_CLANG 0
#endif

// 回退定义（如果没有检测到支持的编译器）
#ifndef CORONA_FORCE_INLINE
    #define CORONA_FORCE_INLINE inline
#endif

#ifndef CORONA_NEVER_INLINE
    #define CORONA_NEVER_INLINE
#endif

#ifndef CORONA_PACKED_STRUCT
    #define CORONA_PACKED_STRUCT(name) struct name
    #define CORONA_PACKED_END
#endif

#ifndef CORONA_FUNCTION_SIGNATURE
    #define CORONA_FUNCTION_SIGNATURE __func__
#endif

#ifndef CORONA_UNREACHABLE
    #define CORONA_UNREACHABLE() do { } while(0)
#endif

#ifndef CORONA_LIKELY
    #define CORONA_LIKELY(x) (x)
    #define CORONA_UNLIKELY(x) (x)
#endif

#ifndef CORONA_RESTRICT
    #define CORONA_RESTRICT
#endif

#ifndef CORONA_THREAD_LOCAL
    #include <thread>
    #define CORONA_THREAD_LOCAL thread_local
#endif

// ========================================
// 平台检测宏 (Platform Detection)
// ========================================

#ifdef CORONA_PLATFORM_WINDOWS
    #define CORONA_WINDOWS 1
    #define CORONA_PLATFORM_NAME "Windows"
    #define CORONA_PATH_SEPARATOR '\\'
    #define CORONA_PATH_SEPARATOR_STR "\\"
    #define CORONA_LINE_ENDING "\r\n"
#else
    #define CORONA_WINDOWS 0
#endif

#ifdef CORONA_PLATFORM_LINUX
    #define CORONA_LINUX 1
    #define CORONA_PLATFORM_NAME "Linux"
    #define CORONA_PATH_SEPARATOR '/'
    #define CORONA_PATH_SEPARATOR_STR "/"
    #define CORONA_LINE_ENDING "\n"
#else
    #define CORONA_LINUX 0
#endif

#ifdef CORONA_PLATFORM_MACOS
    #define CORONA_MACOS 1
    #define CORONA_PLATFORM_NAME "macOS"
    #define CORONA_PATH_SEPARATOR '/'
    #define CORONA_PATH_SEPARATOR_STR "/"
    #define CORONA_LINE_ENDING "\n"
#else
    #define CORONA_MACOS 0
#endif

#ifndef CORONA_PLATFORM_NAME
    #define CORONA_PLATFORM_NAME "Unknown"
    #define CORONA_PATH_SEPARATOR '/'
    #define CORONA_PATH_SEPARATOR_STR "/"
    #define CORONA_LINE_ENDING "\n"
#endif

// ========================================
// 架构检测宏 (Architecture Detection)
// ========================================

#ifdef CORONA_ARCH_64BIT
    #define CORONA_64BIT 1
    #define CORONA_32BIT 0
    #define CORONA_ARCH_NAME "64-bit"
    #define CORONA_POINTER_SIZE 8
#elif defined(CORONA_ARCH_32BIT)
    #define CORONA_64BIT 0
    #define CORONA_32BIT 1
    #define CORONA_ARCH_NAME "32-bit"
    #define CORONA_POINTER_SIZE 4
#else
    // 运行时检测（回退方案）
    #define CORONA_64BIT (sizeof(void*) == 8)
    #define CORONA_32BIT (sizeof(void*) == 4)
    #define CORONA_ARCH_NAME (sizeof(void*) == 8 ? "64-bit" : "32-bit")
    #define CORONA_POINTER_SIZE sizeof(void*)
#endif

// ========================================
// 便利组合宏 (Convenience Macros)
// ========================================

// 检查是否为Microsoft编译器（MSVC或Clang-CL）
#define CORONA_COMPILER_MICROSOFT (CORONA_MSVC || defined(_MSC_VER))

// 检查是否为类Unix平台
#define CORONA_PLATFORM_UNIX (CORONA_LINUX || CORONA_MACOS)

// 检查是否支持GCC风格的属性
#define CORONA_HAS_GCC_ATTRIBUTES (CORONA_GCC || CORONA_CLANG || CORONA_APPLE_CLANG)

// 检查是否为移动平台（将来可扩展）
#define CORONA_PLATFORM_MOBILE 0

// 检查是否为桌面平台
#define CORONA_PLATFORM_DESKTOP (CORONA_WINDOWS || CORONA_LINUX || CORONA_MACOS)

// ========================================
// 编译器版本宏 (Compiler Version)
// ========================================

#if CORONA_MSVC
    #define CORONA_COMPILER_NAME "MSVC"
    #define CORONA_COMPILER_VERSION _MSC_VER
    #define CORONA_COMPILER_VERSION_MAJOR (_MSC_VER / 100)
    #define CORONA_COMPILER_VERSION_MINOR ((_MSC_VER % 100) / 10)
#elif CORONA_GCC
    #define CORONA_COMPILER_NAME "GCC"
    #define CORONA_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #define CORONA_COMPILER_VERSION_MAJOR __GNUC__
    #define CORONA_COMPILER_VERSION_MINOR __GNUC_MINOR__
#elif CORONA_CLANG
    #define CORONA_COMPILER_NAME "Clang"
    #define CORONA_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
    #define CORONA_COMPILER_VERSION_MAJOR __clang_major__
    #define CORONA_COMPILER_VERSION_MINOR __clang_minor__
#elif CORONA_APPLE_CLANG
    #define CORONA_COMPILER_NAME "Apple Clang"
    #define CORONA_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
    #define CORONA_COMPILER_VERSION_MAJOR __clang_major__
    #define CORONA_COMPILER_VERSION_MINOR __clang_minor__
#else
    #define CORONA_COMPILER_NAME "Unknown"
    #define CORONA_COMPILER_VERSION 0
    #define CORONA_COMPILER_VERSION_MAJOR 0
    #define CORONA_COMPILER_VERSION_MINOR 0
#endif

// ========================================
// 调试和发布构建检测 (Debug/Release Detection)
// ========================================

#if defined(CORONA_ENGINE_DEBUG)
    #define CORONA_DEBUG 1
    #define CORONA_RELEASE 0
    #define CORONA_BUILD_TYPE_NAME "Debug"
#elif defined(CORONA_ENGINE_RELEASE)
    #define CORONA_DEBUG 0
    #define CORONA_RELEASE 1
    #define CORONA_BUILD_TYPE_NAME "Release"
#else
    // 回退检测（基于标准宏）
    #if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
        #define CORONA_DEBUG 1
        #define CORONA_RELEASE 0
        #define CORONA_BUILD_TYPE_NAME "Debug"
    #else
        #define CORONA_DEBUG 0
        #define CORONA_RELEASE 1
        #define CORONA_BUILD_TYPE_NAME "Release"
    #endif
#endif

// ========================================
// 实用工具宏 (Utility Macros)
// ========================================

// 字符串化宏
#define CORONA_STRINGIFY(x) #x
#define CORONA_STRINGIFY_EXPANDED(x) CORONA_STRINGIFY(x)

// 连接宏
#define CORONA_CONCAT(a, b) a ## b
#define CORONA_CONCAT_EXPANDED(a, b) CORONA_CONCAT(a, b)

// 数组大小宏
#define CORONA_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// 对齐宏
#if CORONA_MSVC
    #define CORONA_ALIGN(n) __declspec(align(n))
#elif CORONA_HAS_GCC_ATTRIBUTES
    #define CORONA_ALIGN(n) __attribute__((aligned(n)))
#else
    #define CORONA_ALIGN(n)
#endif

// 废弃标记宏
#if CORONA_MSVC
    #define CORONA_DEPRECATED(msg) __declspec(deprecated(msg))
#elif CORONA_HAS_GCC_ATTRIBUTES
    #define CORONA_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
    #define CORONA_DEPRECATED(msg)
#endif

// 无返回值函数标记
#if CORONA_MSVC
    #define CORONA_NORETURN __declspec(noreturn)
#elif CORONA_HAS_GCC_ATTRIBUTES
    #define CORONA_NORETURN __attribute__((noreturn))
#else
    #define CORONA_NORETURN
#endif

// 可能未使用变量标记
#if CORONA_HAS_GCC_ATTRIBUTES
    #define CORONA_MAYBE_UNUSED __attribute__((unused))
#else
    #define CORONA_MAYBE_UNUSED
#endif

// 热路径标记（用于性能优化）
#if CORONA_HAS_GCC_ATTRIBUTES
    #define CORONA_HOT __attribute__((hot))
    #define CORONA_COLD __attribute__((cold))
#else
    #define CORONA_HOT
    #define CORONA_COLD
#endif

// ========================================
// 版本信息 (Version Information)  
// ========================================

// CoronaEngine 版本信息（将由 CMake 自动更新）
#ifndef CORONA_ENGINE_VERSION_MAJOR
    #define CORONA_ENGINE_VERSION_MAJOR 0
#endif

#ifndef CORONA_ENGINE_VERSION_MINOR
    #define CORONA_ENGINE_VERSION_MINOR 5
#endif

#ifndef CORONA_ENGINE_VERSION_PATCH
    #define CORONA_ENGINE_VERSION_PATCH 0
#endif

#define CORONA_ENGINE_VERSION_STRING \
    CORONA_STRINGIFY_EXPANDED(CORONA_ENGINE_VERSION_MAJOR) "." \
    CORONA_STRINGIFY_EXPANDED(CORONA_ENGINE_VERSION_MINOR) "." \
    CORONA_STRINGIFY_EXPANDED(CORONA_ENGINE_VERSION_PATCH)

// ========================================
// 编译时环境信息宏 (Compile-time Environment)
// ========================================

#define CORONA_COMPILE_INFO \
    "Corona Engine " CORONA_ENGINE_VERSION_STRING \
    " (" CORONA_COMPILER_NAME " " CORONA_STRINGIFY_EXPANDED(CORONA_COMPILER_VERSION) \
    ", " CORONA_PLATFORM_NAME " " CORONA_ARCH_NAME \
    ", " CORONA_BUILD_TYPE_NAME ")"

// ========================================
// 使用示例 (Usage Examples)
// ========================================
/*

// 跨平台的内联函数定义
CORONA_FORCE_INLINE int fast_function() {
    return 42;
}

// 跨平台的紧密打包结构体
CORONA_PACKED_STRUCT(MyPackedStruct) {
    char a;
    int b;
} CORONA_PACKED_END;

// 性能优化提示
CORONA_HOT CORONA_FORCE_INLINE int hot_function(int x) {
    if (CORONA_LIKELY(x > 0)) {
        return x * 2;
    }
    return 0;
}

// 编译器特定代码
#if CORONA_MSVC
    // MSVC特定实现
    #pragma warning(disable: 4996)
#elif CORONA_GCC
    // GCC特定实现
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif CORONA_CLANG
    // Clang特定实现
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

// 平台特定代码
#if CORONA_WINDOWS
    #include <windows.h>
    void windows_specific_function() {
        // Windows 特定代码
    }
#elif CORONA_LINUX
    #include <unistd.h>
    void linux_specific_function() {
        // Linux 特定代码
    }
#elif CORONA_MACOS
    #include <mach/mach.h>
    void macos_specific_function() {
        // macOS 特定代码
    }
#endif

// 调试和发布构建
#if CORONA_DEBUG
    #define DBG_PRINT(msg) printf("DEBUG: %s\n", msg)
#else
    #define DBG_PRINT(msg) do {} while(0)
#endif

// 版本检查
void print_engine_info() {
    printf("%s\n", CORONA_COMPILE_INFO);
}

*/