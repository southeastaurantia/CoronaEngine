# Compiler Features API 文档

## 概述

`compiler_features.h` 是 CoronaEngine 的跨平台编译器特性检测与便捷宏集合。该文件**完全独立**于 CMake 配置，仅依赖编译器和平台内置宏，可在任何 C++14+ 项目中独立使用。

**文件路径**: `src/common/include/compiler_features.h`

## 设计理念

- **零依赖**：不依赖 CMake 生成的配置宏
- **编译时检测**：所有特性在预处理阶段确定
- **跨平台统一**：提供统一的宏接口，屏蔽编译器差异
- **可扩展**：易于添加新的编译器和平台支持

---

## 编译器检测

### 基础编译器识别

| 宏 | 描述 | 检测条件 |
|---|---|---|
| `CE_BUILTIN_COMPILER_CLANG` | Clang/AppleClang 编译器 | `__clang__` |
| `CE_BUILTIN_COMPILER_GCC` | GCC 编译器（不包括 Clang） | `__GNUC__ && !__clang__` |
| `CE_BUILTIN_COMPILER_MSVC` | Microsoft Visual C++ | `_MSC_VER && !__clang__` |
| `CE_BUILTIN_COMPILER_INTEL` | Intel C++ 编译器 | `__INTEL_COMPILER` 或 `__INTEL_LLVM_COMPILER` |
| `CE_BUILTIN_COMPILER_EMSCRIPTEN` | Emscripten (WebAssembly) | `__EMSCRIPTEN__` |

### 编译器家族

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_COMPILER_CLANG_CL` | Clang-cl（MSVC 兼容模式的 Clang） |
| `CE_BUILTIN_COMPILER_LLVM_FAMILY` | LLVM 家族（Clang + Intel） |
| `CE_BUILTIN_COMPILER_GCC_FAMILY` | GCC 家族（GCC + LLVM 家族） |

### C++ 标准检测

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_CPLUSPLUS` | 当前 C++ 标准版本号 |
| `CE_BUILTIN_CPP14` | C++14 或更高 |
| `CE_BUILTIN_CPP17` | C++17 或更高 |
| `CE_BUILTIN_CPP20` | C++20 或更高 |
| `CE_BUILTIN_CPP23` | C++23 或更高 |

**示例**:
```cpp
#if CE_BUILTIN_CPP17
    // 使用 C++17 特性
    auto [x, y] = get_pair();
#endif
```

### 编译器信息字符串

| 宏 | 描述 | 示例 |
|---|---|---|
| `CE_BUILTIN_COMPILER_NAME` | 编译器名称 | `"Clang"`, `"GCC"`, `"MSVC"` |
| `CE_BUILTIN_COMPILER_VERSION` | 编译器版本号 | `140001` (Clang 14.0.1) |
| `CE_BUILTIN_COMPILE_INFO` | 完整编译信息 | `"Windows / x86_64 / Clang-140001"` |

---

## 平台检测

### 操作系统

| 宏 | 描述 | 检测条件 |
|---|---|---|
| `CE_BUILTIN_PLATFORM_WINDOWS` | Windows 平台 | `_WIN32` 或 `_WIN64` |
| `CE_BUILTIN_PLATFORM_LINUX` | Linux (不含 Android) | `__linux__ && !__ANDROID__` |
| `CE_BUILTIN_PLATFORM_MACOS` | macOS | `TARGET_OS_OSX` |
| `CE_BUILTIN_PLATFORM_IOS` | iOS/iPadOS | `TARGET_OS_IPHONE` |
| `CE_BUILTIN_PLATFORM_ANDROID` | Android | `__ANDROID__` |
| `CE_BUILTIN_PLATFORM_WASM` | WebAssembly | `__EMSCRIPTEN__` |

### 平台组合

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_PLATFORM_POSIX` | POSIX 兼容平台（Linux/macOS/Android） |
| `CE_BUILTIN_PLATFORM_DESKTOP` | 桌面平台（Windows/Linux/macOS） |
| `CE_BUILTIN_PLATFORM_MOBILE` | 移动平台（Android/iOS） |

### 平台特定常量

| 宏 | Windows | POSIX |
|---|---|---|
| `CE_BUILTIN_PATH_SEPARATOR` | `'\\'` | `'/'` |
| `CE_BUILTIN_PATH_SEPARATOR_STR` | `"\\"` | `"/"` |
| `CE_BUILTIN_LINE_ENDING` | `"\r\n"` | `"\n"` |

**示例**:
```cpp
std::string path = "data" CE_BUILTIN_PATH_SEPARATOR_STR "config.json";
// Windows: "data\\config.json"
// Linux: "data/config.json"
```

### 平台名称

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_PLATFORM_NAME` | 平台名称字符串 (`"Windows"`, `"Linux"`, `"macOS"` 等) |

---

## 架构检测

### CPU 架构

| 宏 | 描述 | 检测条件 |
|---|---|---|
| `CE_BUILTIN_ARCH_X86_64` | x86-64 (AMD64) | `_M_X64`, `__x86_64__`, `__amd64__` |
| `CE_BUILTIN_ARCH_X86_32` | x86 (IA-32) | `_M_IX86`, `__i386__` |
| `CE_BUILTIN_ARCH_ARM64` | ARM64 (AArch64) | `_M_ARM64`, `__aarch64__` |
| `CE_BUILTIN_ARCH_ARM32` | ARM32 | `_M_ARM`, `__arm__` |
| `CE_BUILTIN_ARCH_RISCV` | RISC-V | `__riscv`, `__riscv__` |

### 位宽

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_ARCH_64BIT` | 64 位架构 |
| `CE_BUILTIN_ARCH_32BIT` | 32 位架构 |
| `CE_BUILTIN_POINTER_SIZE` | 指针大小（字节）：8 或 4 |

### 架构名称

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_ARCH_NAME` | 架构名称字符串 (`"x86_64"`, `"ARM64"` 等) |

---

## 属性检测辅助宏

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_HAS_ATTRIBUTE(x)` | 检测编译器是否支持 `__attribute__((x))` |
| `CE_BUILTIN_HAS_CPP_ATTRIBUTE(x)` | 检测是否支持 `[[x]]` C++ 属性 |
| `CE_BUILTIN_HAS_INCLUDE(x)` | 检测头文件是否存在 |

**示例**:
```cpp
#if CE_BUILTIN_HAS_INCLUDE(<optional>)
    #include <optional>
#else
    #include "optional_polyfill.h"
#endif
```

---

## 通用属性宏

### 内联控制

| 宏 | 描述 | MSVC | GCC/Clang |
|---|---|---|---|
| `CE_BUILTIN_FORCE_INLINE` | 强制内联 | `__forceinline` | `__attribute__((always_inline))` |
| `CE_BUILTIN_NEVER_INLINE` | 禁止内联 | `__declspec(noinline)` | `__attribute__((noinline))` |

**示例**:
```cpp
CE_BUILTIN_FORCE_INLINE int fast_add(int a, int b) {
    return a + b;
}
```

### 优化提示

| 宏 | 描述 | 用途 |
|---|---|---|
| `CE_BUILTIN_LIKELY(x)` | 提示分支很可能为真 | 热路径优化 |
| `CE_BUILTIN_UNLIKELY(x)` | 提示分支很可能为假 | 错误处理优化 |
| `CE_BUILTIN_ASSUME(x)` | 告知编译器假设为真 | 优化前提 |
| `CE_BUILTIN_UNREACHABLE()` | 标记代码不可达 | 帮助死代码消除 |

**示例**:
```cpp
if (CE_BUILTIN_LIKELY(ptr != nullptr)) {
    // 快速路径
    process(ptr);
} else {
    // 异常路径
    handle_error();
}
```

### C++17 属性

| 宏 | 描述 | C++ 等价 |
|---|---|---|
| `CE_BUILTIN_CPP_LIKELY` | C++20 likely 属性 | `[[likely]]` |
| `CE_BUILTIN_CPP_UNLIKELY` | C++20 unlikely 属性 | `[[unlikely]]` |
| `CE_BUILTIN_NODISCARD` | 标记返回值不应被忽略 | `[[nodiscard]]` |
| `CE_BUILTIN_FALLTHROUGH` | switch 语句穿透标记 | `[[fallthrough]]` |
| `CE_BUILTIN_MAYBE_UNUSED` | 标记可能未使用的变量 | `[[maybe_unused]]` |

**示例**:
```cpp
CE_BUILTIN_NODISCARD int calculate() { return 42; }

switch (value) {
    case 1:
        foo();
        CE_BUILTIN_FALLTHROUGH; // 明确告知这是故意穿透
    case 2:
        bar();
        break;
}
```

### 函数属性

| 宏 | 描述 | 用途 |
|---|---|---|
| `CE_BUILTIN_HOT` | 标记热点函数 | 优化频繁调用的代码 |
| `CE_BUILTIN_COLD` | 标记冷函数 | 优化尺寸而非速度 |
| `CE_BUILTIN_NORETURN` | 函数不返回 | 如 `abort()`, `exit()` |
| `CE_BUILTIN_DEPRECATED(msg)` | 标记过时 API | 弃用警告 |

**示例**:
```cpp
CE_BUILTIN_HOT void render_loop() {
    // 每帧调用的热路径
}

CE_BUILTIN_COLD void log_error(const char* msg) {
    // 异常情况下才调用
}

CE_BUILTIN_NORETURN void fatal_error(const char* msg) {
    std::fprintf(stderr, "%s\n", msg);
    std::abort();
}
```

### 内存与线程

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_ALIGN(n)` | 指定对齐字节数 |
| `CE_BUILTIN_RESTRICT` | restrict 关键字（指针别名优化） |
| `CE_BUILTIN_THREAD_LOCAL` | 线程局部存储 |

**示例**:
```cpp
// 缓存行对齐
CE_BUILTIN_ALIGN(64) struct CacheLinePadded {
    std::atomic<int> counter;
};

// 指针别名优化
void copy(CE_BUILTIN_RESTRICT float* dst, 
          CE_BUILTIN_RESTRICT const float* src, 
          size_t n);

// 线程局部变量
CE_BUILTIN_THREAD_LOCAL int thread_id = 0;
```

---

## 导出/导入可见性

| 宏 | 描述 | Windows | GCC/Clang |
|---|---|---|---|
| `CE_BUILTIN_EXPORT` | 导出符号 | `__declspec(dllexport)` | `__attribute__((visibility("default")))` |
| `CE_BUILTIN_IMPORT` | 导入符号 | `__declspec(dllimport)` | `__attribute__((visibility("default")))` |
| `CE_BUILTIN_LOCAL` | 隐藏符号 | (无) | `__attribute__((visibility("hidden")))` |

**示例**:
```cpp
// 在库的公共头文件中
#ifdef MYLIB_EXPORTS
    #define MYLIB_API CE_BUILTIN_EXPORT
#else
    #define MYLIB_API CE_BUILTIN_IMPORT
#endif

MYLIB_API void public_function();
CE_BUILTIN_LOCAL void internal_helper();
```

---

## 实用宏

### 预处理器工具

| 宏 | 描述 | 示例 |
|---|---|---|
| `CE_BUILTIN_STRINGIFY(x)` | 转为字符串字面量 | `STRINGIFY(foo)` → `"foo"` |
| `CE_BUILTIN_STRINGIFY_EXPANDED(x)` | 展开后转字符串 | `STRINGIFY_EXPANDED(__LINE__)` → `"42"` |
| `CE_BUILTIN_CONCAT(a, b)` | 连接标记 | `CONCAT(foo, bar)` → `foobar` |
| `CE_BUILTIN_CONCAT_EXPANDED(a, b)` | 展开后连接 | |
| `CE_BUILTIN_ARRAY_SIZE(arr)` | 计算数组元素数 | `ARRAY_SIZE(arr)` → `sizeof(arr)/sizeof(arr[0])` |

**示例**:
```cpp
#define VERSION_MAJOR 1
#define VERSION_MINOR 2

// 错误：STRINGIFY(VERSION_MAJOR) → "VERSION_MAJOR"
// 正确：
const char* version = CE_BUILTIN_STRINGIFY_EXPANDED(VERSION_MAJOR) "." 
                      CE_BUILTIN_STRINGIFY_EXPANDED(VERSION_MINOR);
// 结果: "1.2"

int values[] = {1, 2, 3, 4, 5};
size_t count = CE_BUILTIN_ARRAY_SIZE(values); // 5
```

### 编译时工具

| 宏 | 描述 |
|---|---|
| `CE_BUILTIN_IF_CONSTEXPR(cond)` | C++17 `if constexpr` 简写 |
| `CE_BUILTIN_STATIC_ASSERT(cond, msg)` | 编译时断言 |

**示例**:
```cpp
template<typename T>
void process(T value) {
    CE_BUILTIN_IF_CONSTEXPR(std::is_integral_v<T>) {
        // 整数类型特化
        return value * 2;
    } else {
        // 其他类型
        return value;
    }
}

CE_BUILTIN_STATIC_ASSERT(CE_BUILTIN_ARCH_64BIT, "仅支持 64 位平台");
```

---

## 调试与诊断

### 启用编译信息摘要

定义 `CE_BUILTIN_ENABLE_SUMMARY` 宏后，编译时会输出当前配置：

```cpp
#define CE_BUILTIN_ENABLE_SUMMARY
#include <compiler_features.h>
```

**输出示例**:
```
[CoronaEngine] Windows / x86_64 / Clang-140001
```

---

## 使用场景示例

### 1. 跨平台文件路径

```cpp
#include <compiler_features.h>
#include <string>

std::string get_config_path() {
#if CE_BUILTIN_PLATFORM_WINDOWS
    return "C:\\ProgramData\\MyApp\\config.ini";
#elif CE_BUILTIN_PLATFORM_POSIX
    return "/etc/myapp/config.ini";
#else
    #error "不支持的平台"
#endif
}
```

### 2. SIMD 检测与优化

```cpp
#include <compiler_features.h>

void process_array(float* data, size_t n) {
#if CE_BUILTIN_ARCH_X86_64 && CE_BUILTIN_HAS_INCLUDE(<immintrin.h>)
    // 使用 AVX2
    #include <immintrin.h>
    process_with_avx2(data, n);
#elif CE_BUILTIN_ARCH_ARM64 && CE_BUILTIN_HAS_INCLUDE(<arm_neon.h>)
    // 使用 NEON
    #include <arm_neon.h>
    process_with_neon(data, n);
#else
    // 标量回退
    process_scalar(data, n);
#endif
}
```

### 3. 性能关键代码优化

```cpp
#include <compiler_features.h>

CE_BUILTIN_HOT CE_BUILTIN_FORCE_INLINE
int search_sorted(const int* arr, int n, int target) {
    int left = 0, right = n - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (CE_BUILTIN_LIKELY(arr[mid] < target)) {
            left = mid + 1;
        } else if (arr[mid] > target) {
            right = mid - 1;
        } else {
            return mid;
        }
    }
    
    return -1;
}
```

### 4. 错误处理

```cpp
#include <compiler_features.h>
#include <cstdio>

CE_BUILTIN_COLD CE_BUILTIN_NORETURN
void panic(const char* msg) {
    std::fprintf(stderr, "致命错误: %s\n", msg);
    std::fprintf(stderr, "平台: %s\n", CE_BUILTIN_PLATFORM_NAME);
    std::fprintf(stderr, "编译器: %s\n", CE_BUILTIN_COMPILER_NAME);
    std::abort();
}

void safe_divide(int a, int b) {
    if (CE_BUILTIN_UNLIKELY(b == 0)) {
        panic("除零错误");
        CE_BUILTIN_UNREACHABLE();
    }
    return a / b;
}
```

### 5. 版本兼容性检查

```cpp
#include <compiler_features.h>

// 确保最低 C++17
CE_BUILTIN_STATIC_ASSERT(CE_BUILTIN_CPP17, "需要 C++17 或更高版本");

// 可选功能检测
#if CE_BUILTIN_CPP20
    #include <concepts>
    #include <ranges>
    #define HAS_MODERN_FEATURES 1
#else
    #define HAS_MODERN_FEATURES 0
#endif
```

---

## 最佳实践

### ✅ 推荐做法

1. **优先使用统一宏**：使用 `CE_BUILTIN_FORCE_INLINE` 而非直接写 `__forceinline` 或 `__attribute__((always_inline))`
2. **分支预测适度使用**：仅在性能关键路径使用 `LIKELY`/`UNLIKELY`
3. **编译时检查**：用 `STATIC_ASSERT` 在编译期捕获配置错误
4. **组合宏判断**：使用 `CE_BUILTIN_PLATFORM_POSIX` 代替 `CE_BUILTIN_PLATFORM_LINUX || CE_BUILTIN_PLATFORM_MACOS`

### ❌ 避免做法

1. **过度优化**：不要在普通代码中滥用 `FORCE_INLINE` 和 `HOT`
2. **重复检测**：不要定义自己的平台检测宏，直接使用库提供的
3. **硬编码假设**：不要假设所有平台都是 64 位或小端序

---

## 扩展指南

### 添加新平台

在 `compiler_features.h` 中添加：

```cpp
// 平台检测
#if defined(__CUSTOM_PLATFORM__)
    #define CE_BUILTIN_PLATFORM_CUSTOM 1
#else
    #define CE_BUILTIN_PLATFORM_CUSTOM 0
#endif

// 平台名称
#elif CE_BUILTIN_PLATFORM_CUSTOM
    #define CE_BUILTIN_PLATFORM_NAME "CustomPlatform"
```

### 添加新编译器

```cpp
// 编译器检测
#if defined(__CUSTOM_COMPILER__)
    #define CE_BUILTIN_COMPILER_CUSTOM 1
#else
    #define CE_BUILTIN_COMPILER_CUSTOM 0
#endif

// 编译器信息
#elif CE_BUILTIN_COMPILER_CUSTOM
    #define CE_BUILTIN_COMPILER_NAME "CustomCompiler"
    #define CE_BUILTIN_COMPILER_VERSION __CUSTOM_VERSION__
```

---

## 参考资料

- [GCC Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [Clang Attributes](https://clang.llvm.org/docs/AttributeReference.html)
- [MSVC Attributes](https://docs.microsoft.com/en-us/cpp/cpp/attributes)
- [C++ Standard Attributes](https://en.cppreference.com/w/cpp/language/attributes)

---

**最后更新**: 2025-10-05  
**引擎版本**: CoronaEngine 0.5.0  
**维护者**: CoronaEngine Team
