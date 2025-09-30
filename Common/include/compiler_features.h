#pragma once

/**
 * @file portable_compiler_features.h
 * @brief 基于标准/编译器内置宏的跨平台特性检测与便捷宏集合。
 *
 * 本文件仅依赖于编译器和平台自身提供的内置宏，不需要 CoronaEngine CMake
 * 生成的任何配置宏即可独立使用。它提供常见的编译器、平台与架构检测，
 * 以及一组跨平台统一的函数/属性装饰宏，便于在引擎基础设施中直接复用。
 */

// ========================================
// 编译器检测 (Compiler Detection)
// ========================================

#if defined(__clang__)
    #define CE_BUILTIN_COMPILER_CLANG 1
#else
    #define CE_BUILTIN_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
    #define CE_BUILTIN_COMPILER_GCC 1
#else
    #define CE_BUILTIN_COMPILER_GCC 0
#endif

#if defined(_MSC_VER) && !defined(__clang__)
    #define CE_BUILTIN_COMPILER_MSVC 1
#else
    #define CE_BUILTIN_COMPILER_MSVC 0
#endif

#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
    #define CE_BUILTIN_COMPILER_INTEL 1
#else
    #define CE_BUILTIN_COMPILER_INTEL 0
#endif

#if defined(__EMSCRIPTEN__)
    #define CE_BUILTIN_COMPILER_EMSCRIPTEN 1
#else
    #define CE_BUILTIN_COMPILER_EMSCRIPTEN 0
#endif

#define CE_BUILTIN_COMPILER_CLANG_CL (CE_BUILTIN_COMPILER_CLANG && defined(_MSC_VER))
#define CE_BUILTIN_COMPILER_LLVM_FAMILY (CE_BUILTIN_COMPILER_CLANG || CE_BUILTIN_COMPILER_INTEL)
#define CE_BUILTIN_COMPILER_GCC_FAMILY (CE_BUILTIN_COMPILER_GCC || CE_BUILTIN_COMPILER_LLVM_FAMILY)

#if defined(_MSVC_LANG)
    #define CE_BUILTIN_CPLUSPLUS _MSVC_LANG
#elif defined(__cplusplus)
    #define CE_BUILTIN_CPLUSPLUS __cplusplus
#else
    #define CE_BUILTIN_CPLUSPLUS 0L
#endif

#define CE_BUILTIN_CPP_STANDARD(v) (CE_BUILTIN_CPLUSPLUS >= (v))
#define CE_BUILTIN_CPP14 CE_BUILTIN_CPP_STANDARD(201402L)
#define CE_BUILTIN_CPP17 CE_BUILTIN_CPP_STANDARD(201703L)
#define CE_BUILTIN_CPP20 CE_BUILTIN_CPP_STANDARD(202002L)
#define CE_BUILTIN_CPP23 CE_BUILTIN_CPP_STANDARD(202302L)

// ========================================
// 平台检测 (Platform Detection)
// ========================================

#if defined(_WIN32) || defined(_WIN64)
    #define CE_BUILTIN_PLATFORM_WINDOWS 1
#else
    #define CE_BUILTIN_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__)
    #define CE_BUILTIN_PLATFORM_APPLE 1
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_OS_IPAD
        #define CE_BUILTIN_PLATFORM_IOS 1
    #else
        #define CE_BUILTIN_PLATFORM_IOS 0
    #endif
    #if TARGET_OS_OSX
        #define CE_BUILTIN_PLATFORM_MACOS 1
    #else
        #define CE_BUILTIN_PLATFORM_MACOS 0
    #endif
#else
    #define CE_BUILTIN_PLATFORM_APPLE 0
    #define CE_BUILTIN_PLATFORM_IOS 0
    #define CE_BUILTIN_PLATFORM_MACOS 0
#endif

#if defined(__ANDROID__)
    #define CE_BUILTIN_PLATFORM_ANDROID 1
#else
    #define CE_BUILTIN_PLATFORM_ANDROID 0
#endif

#if defined(__linux__) && !defined(__ANDROID__)
    #define CE_BUILTIN_PLATFORM_LINUX 1
#else
    #define CE_BUILTIN_PLATFORM_LINUX 0
#endif

#if defined(__EMSCRIPTEN__)
    #define CE_BUILTIN_PLATFORM_WASM 1
#else
    #define CE_BUILTIN_PLATFORM_WASM 0
#endif

#define CE_BUILTIN_PLATFORM_POSIX (CE_BUILTIN_PLATFORM_LINUX || CE_BUILTIN_PLATFORM_MACOS || CE_BUILTIN_PLATFORM_ANDROID)
#define CE_BUILTIN_PLATFORM_DESKTOP (CE_BUILTIN_PLATFORM_WINDOWS || CE_BUILTIN_PLATFORM_LINUX || CE_BUILTIN_PLATFORM_MACOS)
#define CE_BUILTIN_PLATFORM_MOBILE (CE_BUILTIN_PLATFORM_ANDROID || CE_BUILTIN_PLATFORM_IOS)

#if CE_BUILTIN_PLATFORM_WINDOWS
    #define CE_BUILTIN_PATH_SEPARATOR '\\'
    #define CE_BUILTIN_PATH_SEPARATOR_STR "\\"
    #define CE_BUILTIN_LINE_ENDING "\r\n"
#elif CE_BUILTIN_PLATFORM_POSIX
    #define CE_BUILTIN_PATH_SEPARATOR '/'
    #define CE_BUILTIN_PATH_SEPARATOR_STR "/"
    #define CE_BUILTIN_LINE_ENDING "\n"
#else
    #define CE_BUILTIN_PATH_SEPARATOR '/'
    #define CE_BUILTIN_PATH_SEPARATOR_STR "/"
    #define CE_BUILTIN_LINE_ENDING "\n"
#endif

// ========================================
// 架构检测 (Architecture Detection)
// ========================================

#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
    #define CE_BUILTIN_ARCH_X86_64 1
#else
    #define CE_BUILTIN_ARCH_X86_64 0
#endif

#if defined(_M_IX86) || defined(__i386__)
    #define CE_BUILTIN_ARCH_X86_32 1
#else
    #define CE_BUILTIN_ARCH_X86_32 0
#endif

#if defined(_M_ARM64) || defined(__aarch64__)
    #define CE_BUILTIN_ARCH_ARM64 1
#else
    #define CE_BUILTIN_ARCH_ARM64 0
#endif

#if defined(_M_ARM) || defined(__arm__)
    #define CE_BUILTIN_ARCH_ARM32 1
#else
    #define CE_BUILTIN_ARCH_ARM32 0
#endif

#if defined(__riscv) || defined(__riscv__)
    #define CE_BUILTIN_ARCH_RISCV 1
#else
    #define CE_BUILTIN_ARCH_RISCV 0
#endif

#if CE_BUILTIN_ARCH_X86_64 || CE_BUILTIN_ARCH_ARM64
    #define CE_BUILTIN_ARCH_64BIT 1
    #define CE_BUILTIN_ARCH_32BIT 0
    #define CE_BUILTIN_POINTER_SIZE 8
#elif CE_BUILTIN_ARCH_X86_32 || CE_BUILTIN_ARCH_ARM32
    #define CE_BUILTIN_ARCH_64BIT 0
    #define CE_BUILTIN_ARCH_32BIT 1
    #define CE_BUILTIN_POINTER_SIZE 4
#else
    #define CE_BUILTIN_ARCH_64BIT (sizeof(void*) == 8)
    #define CE_BUILTIN_ARCH_32BIT (sizeof(void*) == 4)
    #define CE_BUILTIN_POINTER_SIZE sizeof(void*)
#endif

// ========================================
// 属性检测辅助宏
// ========================================

#ifndef CE_BUILTIN_HAS_ATTRIBUTE
    #ifdef __has_attribute
        #define CE_BUILTIN_HAS_ATTRIBUTE(x) __has_attribute(x)
    #else
        #define CE_BUILTIN_HAS_ATTRIBUTE(x) 0
    #endif
#endif

#ifndef CE_BUILTIN_HAS_CPP_ATTRIBUTE
    #ifdef __has_cpp_attribute
        #define CE_BUILTIN_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
    #else
        #define CE_BUILTIN_HAS_CPP_ATTRIBUTE(x) 0
    #endif
#endif

#ifndef CE_BUILTIN_HAS_INCLUDE
    #ifdef __has_include
        #define CE_BUILTIN_HAS_INCLUDE(x) __has_include(x)
    #else
        #define CE_BUILTIN_HAS_INCLUDE(x) 0
    #endif
#endif

// ========================================
// 通用属性宏 (General Attributes)
// ========================================

#if CE_BUILTIN_COMPILER_MSVC
    #define CE_BUILTIN_FORCE_INLINE __forceinline
    #define CE_BUILTIN_NEVER_INLINE __declspec(noinline)
    #define CE_BUILTIN_RESTRICT __restrict
    #define CE_BUILTIN_THREAD_LOCAL __declspec(thread)
    #define CE_BUILTIN_ASSUME(x) __assume(x)
#elif CE_BUILTIN_COMPILER_GCC_FAMILY
    #define CE_BUILTIN_FORCE_INLINE inline __attribute__((always_inline))
    #define CE_BUILTIN_NEVER_INLINE __attribute__((noinline))
    #define CE_BUILTIN_RESTRICT __restrict__
    #define CE_BUILTIN_THREAD_LOCAL __thread
    #if CE_BUILTIN_COMPILER_CLANG
        #define CE_BUILTIN_ASSUME(x) __builtin_assume(x)
    #else
        #define CE_BUILTIN_ASSUME(x) (__builtin_expect(!(x), 0) ? __builtin_unreachable() : (void)0)
    #endif
#else
    #define CE_BUILTIN_FORCE_INLINE inline
    #define CE_BUILTIN_NEVER_INLINE
    #define CE_BUILTIN_RESTRICT
    #define CE_BUILTIN_THREAD_LOCAL thread_local
    #define CE_BUILTIN_ASSUME(x) ((void)0)
#endif

#if CE_BUILTIN_COMPILER_MSVC
    #define CE_BUILTIN_UNREACHABLE() __assume(0)
#elif CE_BUILTIN_COMPILER_GCC_FAMILY
    #define CE_BUILTIN_UNREACHABLE() __builtin_unreachable()
#else
    #define CE_BUILTIN_UNREACHABLE() do { } while (false)
#endif

#if CE_BUILTIN_COMPILER_GCC_FAMILY
    #define CE_BUILTIN_LIKELY(x) __builtin_expect(!!(x), 1)
    #define CE_BUILTIN_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define CE_BUILTIN_LIKELY(x) (x)
    #define CE_BUILTIN_UNLIKELY(x) (x)
#endif

#if CE_BUILTIN_COMPILER_MSVC
    #define CE_BUILTIN_ALIGN(n) __declspec(align(n))
#elif CE_BUILTIN_HAS_ATTRIBUTE(aligned)
    #define CE_BUILTIN_ALIGN(n) __attribute__((aligned(n)))
#else
    #define CE_BUILTIN_ALIGN(n)
#endif

#if CE_BUILTIN_COMPILER_MSVC
    #define CE_BUILTIN_DEPRECATED(msg) __declspec(deprecated(msg))
#elif CE_BUILTIN_HAS_ATTRIBUTE(deprecated)
    #define CE_BUILTIN_DEPRECATED(msg) __attribute__((deprecated(msg)))
#else
    #define CE_BUILTIN_DEPRECATED(msg)
#endif

#if CE_BUILTIN_COMPILER_MSVC
    #define CE_BUILTIN_NORETURN __declspec(noreturn)
#elif CE_BUILTIN_HAS_ATTRIBUTE(noreturn)
    #define CE_BUILTIN_NORETURN __attribute__((noreturn))
#else
    #define CE_BUILTIN_NORETURN
#endif

#if CE_BUILTIN_HAS_ATTRIBUTE(maybe_unused)
    #define CE_BUILTIN_MAYBE_UNUSED __attribute__((maybe_unused))
#elif CE_BUILTIN_HAS_CPP_ATTRIBUTE(maybe_unused)
    #define CE_BUILTIN_MAYBE_UNUSED [[maybe_unused]]
#else
    #define CE_BUILTIN_MAYBE_UNUSED
#endif

#if CE_BUILTIN_HAS_ATTRIBUTE(hot)
    #define CE_BUILTIN_HOT __attribute__((hot))
    #define CE_BUILTIN_COLD __attribute__((cold))
#else
    #define CE_BUILTIN_HOT
    #define CE_BUILTIN_COLD
#endif

#if CE_BUILTIN_HAS_CPP_ATTRIBUTE(likely)
    #define CE_BUILTIN_CPP_LIKELY [[likely]]
    #define CE_BUILTIN_CPP_UNLIKELY [[unlikely]]
#else
    #define CE_BUILTIN_CPP_LIKELY
    #define CE_BUILTIN_CPP_UNLIKELY
#endif

// ========================================
// 导出/导入可见性宏 (Visibility)
// ========================================

#if CE_BUILTIN_PLATFORM_WINDOWS
    #define CE_BUILTIN_EXPORT __declspec(dllexport)
    #define CE_BUILTIN_IMPORT __declspec(dllimport)
    #define CE_BUILTIN_LOCAL
#elif CE_BUILTIN_HAS_ATTRIBUTE(visibility)
    #define CE_BUILTIN_EXPORT __attribute__((visibility("default")))
    #define CE_BUILTIN_IMPORT __attribute__((visibility("default")))
    #define CE_BUILTIN_LOCAL __attribute__((visibility("hidden")))
#else
    #define CE_BUILTIN_EXPORT
    #define CE_BUILTIN_IMPORT
    #define CE_BUILTIN_LOCAL
#endif

// ========================================
// 实用宏 (Utilities)
// ========================================

#define CE_BUILTIN_STRINGIFY(x) #x
#define CE_BUILTIN_STRINGIFY_EXPANDED(x) CE_BUILTIN_STRINGIFY(x)
#define CE_BUILTIN_CONCAT(a, b) a##b
#define CE_BUILTIN_CONCAT_EXPANDED(a, b) CE_BUILTIN_CONCAT(a, b)
#define CE_BUILTIN_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// 快速判断运行时/编译期的工具宏
#define CE_BUILTIN_IF_CONSTEXPR(cond) if constexpr (cond)

#if CE_BUILTIN_CPP17
    #define CE_BUILTIN_NODISCARD [[nodiscard]]
#else
    #define CE_BUILTIN_NODISCARD
#endif

#if CE_BUILTIN_CPP17
    #define CE_BUILTIN_FALLTHROUGH [[fallthrough]]
#elif CE_BUILTIN_HAS_CPP_ATTRIBUTE(fallthrough)
    #define CE_BUILTIN_FALLTHROUGH [[fallthrough]]
#elif CE_BUILTIN_HAS_ATTRIBUTE(fallthrough)
    #define CE_BUILTIN_FALLTHROUGH __attribute__((fallthrough))
#else
    #define CE_BUILTIN_FALLTHROUGH ((void)0)
#endif

// ========================================
// 编译信息字符串
// ========================================

#if CE_BUILTIN_COMPILER_MSVC
    #define CE_BUILTIN_COMPILER_NAME "MSVC"
    #define CE_BUILTIN_COMPILER_VERSION _MSC_VER
#elif CE_BUILTIN_COMPILER_CLANG
    #define CE_BUILTIN_COMPILER_NAME "Clang"
    #define CE_BUILTIN_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif CE_BUILTIN_COMPILER_GCC
    #define CE_BUILTIN_COMPILER_NAME "GCC"
    #define CE_BUILTIN_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif CE_BUILTIN_COMPILER_INTEL
    #define CE_BUILTIN_COMPILER_NAME "Intel"
    #ifdef __INTEL_COMPILER
        #define CE_BUILTIN_COMPILER_VERSION __INTEL_COMPILER
    #else
        #define CE_BUILTIN_COMPILER_VERSION __INTEL_LLVM_COMPILER
    #endif
#else
    #define CE_BUILTIN_COMPILER_NAME "Unknown"
    #define CE_BUILTIN_COMPILER_VERSION 0
#endif

#if CE_BUILTIN_PLATFORM_WINDOWS
    #define CE_BUILTIN_PLATFORM_NAME "Windows"
#elif CE_BUILTIN_PLATFORM_LINUX
    #define CE_BUILTIN_PLATFORM_NAME "Linux"
#elif CE_BUILTIN_PLATFORM_MACOS
    #define CE_BUILTIN_PLATFORM_NAME "macOS"
#elif CE_BUILTIN_PLATFORM_ANDROID
    #define CE_BUILTIN_PLATFORM_NAME "Android"
#elif CE_BUILTIN_PLATFORM_IOS
    #define CE_BUILTIN_PLATFORM_NAME "iOS"
#elif CE_BUILTIN_PLATFORM_WASM
    #define CE_BUILTIN_PLATFORM_NAME "WebAssembly"
#else
    #define CE_BUILTIN_PLATFORM_NAME "Unknown"
#endif

#if CE_BUILTIN_ARCH_X86_64
    #define CE_BUILTIN_ARCH_NAME "x86_64"
#elif CE_BUILTIN_ARCH_X86_32
    #define CE_BUILTIN_ARCH_NAME "x86_32"
#elif CE_BUILTIN_ARCH_ARM64
    #define CE_BUILTIN_ARCH_NAME "ARM64"
#elif CE_BUILTIN_ARCH_ARM32
    #define CE_BUILTIN_ARCH_NAME "ARM32"
#elif CE_BUILTIN_ARCH_RISCV
    #define CE_BUILTIN_ARCH_NAME "RISC-V"
#else
    #define CE_BUILTIN_ARCH_NAME "Unknown"
#endif

#define CE_BUILTIN_COMPILE_INFO \
    CE_BUILTIN_PLATFORM_NAME " / " CE_BUILTIN_ARCH_NAME " / " CE_BUILTIN_COMPILER_NAME "-" CE_BUILTIN_STRINGIFY_EXPANDED(CE_BUILTIN_COMPILER_VERSION)

// ========================================
// 断言辅助 (Compile-time guards)
// ========================================

#define CE_BUILTIN_STATIC_ASSERT(cond, msg) static_assert((cond), msg)

// 可选：启用时输出基础配置概览
#if defined(CE_BUILTIN_ENABLE_SUMMARY)
    #pragma message("[CoronaEngine] " CE_BUILTIN_COMPILE_INFO)
#endif
