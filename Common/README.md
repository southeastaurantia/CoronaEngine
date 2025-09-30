# CoronaEngine 全局编译器检测系统

## 概述

CoronaEngine 现在提供了一个全局的编译器检测系统，通过 `Common/include/compiler_detection.h` 头文件为整个项目提供统一的编译器、平台和架构检测宏定义。

## 位置和访问

- **头文件位置**: `Common/include/compiler_detection.h`
- **包含方式**: `#include <compiler_detection.h>`
- **作用域**: 整个 CoronaEngine 项目（包括 Src/、Utility/、Examples/ 等所有模块）

## 核心功能特性

### 1. 编译器检测

支持主流编译器的自动检测和宏定义：

```cpp
// 编译器类型检测
#if CORONA_MSVC
    // MSVC 特定代码
#elif CORONA_GCC  
    // GCC 特定代码
#elif CORONA_CLANG
    // Clang 特定代码
#elif CORONA_APPLE_CLANG
    // Apple Clang 特定代码
#endif

// 编译器信息
std::cout << "编译器: " << CORONA_COMPILER_NAME << std::endl;
std::cout << "版本: " << CORONA_COMPILER_VERSION << std::endl;
```

### 2. 平台检测

跨平台兼容性宏：

```cpp
#if CORONA_WINDOWS
    #include <windows.h>
    // Windows 特定实现
#elif CORONA_LINUX
    #include <unistd.h>  
    // Linux 特定实现
#elif CORONA_MACOS
    #include <mach/mach.h>
    // macOS 特定实现
#endif

// 平台信息
std::cout << "平台: " << CORONA_PLATFORM_NAME << std::endl;
std::cout << "路径分隔符: " << CORONA_PATH_SEPARATOR_STR << std::endl;
```

### 3. 架构检测

```cpp
#if CORONA_64BIT
    std::cout << "64位架构" << std::endl;
#else
    std::cout << "32位架构" << std::endl;  
#endif

std::cout << "指针大小: " << CORONA_POINTER_SIZE << " 字节" << std::endl;
```

### 4. 跨平台紧密打包结构体

```cpp
// 自动适配 MSVC 的 #pragma pack 和 GCC/Clang 的 __attribute__((packed))
CORONA_PACKED_STRUCT(NetworkPacket) {
    uint8_t header;
    uint32_t data;
    uint8_t footer;
} CORONA_PACKED_END;

// 在 MSVC 下等效于：
// #pragma pack(push, 1)
// struct NetworkPacket { ... };
// #pragma pack(pop)

// 在 GCC/Clang 下等效于：
// struct __attribute__((packed)) NetworkPacket { ... };
```

### 5. 性能优化宏

```cpp
// 强制内联（跨编译器）
CORONA_FORCE_INLINE int fast_function() {
    return 42;
}

// 性能提示
CORONA_HOT CORONA_FORCE_INLINE int hot_path(int x) {
    if (CORONA_LIKELY(x > 0)) {
        return x * 2;
    }
    return 0;
}

// 冷路径标记
CORONA_COLD void error_handler() {
    // 错误处理代码
}
```

### 6. 实用工具宏

```cpp
// 编译时信息
std::cout << CORONA_COMPILE_INFO << std::endl;
// 输出: "Corona Engine 0.5.0 (Clang 210101, Windows 64-bit, Debug)"

// 字符串化和连接
#define MY_VERSION 123
std::cout << CORONA_STRINGIFY_EXPANDED(MY_VERSION) << std::endl; // "123"

// 数组大小
int arr[] = {1, 2, 3, 4, 5};
size_t size = CORONA_ARRAY_SIZE(arr); // 5

// 对齐
CORONA_ALIGN(16) struct AlignedStruct {
    float data[4];
};

// 废弃标记
CORONA_DEPRECATED("Use new_function instead")
void old_function() {
    // 旧函数实现
}
```

### 7. 调试和发布构建检测

```cpp
#if CORONA_DEBUG
    #define DBG_LOG(msg) printf("DEBUG: %s\n", msg)
#else
    #define DBG_LOG(msg) do {} while(0)
#endif

std::cout << "构建类型: " << CORONA_BUILD_TYPE_NAME << std::endl;
```

## CMake 集成

系统自动通过 CMake 在 `CoronaCompileConfig.cmake` 中注入编译器检测宏：

- `CORONA_COMPILER_MSVC`、`CORONA_COMPILER_GCC`、`CORONA_COMPILER_CLANG`
- `CORONA_PLATFORM_WINDOWS`、`CORONA_PLATFORM_LINUX`、`CORONA_PLATFORM_MACOS`  
- `CORONA_ARCH_64BIT`、`CORONA_ARCH_32BIT`
- `CORONA_ENGINE_DEBUG`、`CORONA_ENGINE_RELEASE`

## 项目集成情况

### 自动包含路径

- **CoronaEngine 主库**: 已自动包含 `Common/include` 路径
- **Utility 模块**: 所有 Utility 子模块自动可访问
- **Examples**: 通过 CoronaEngine 库依赖自动可用

### 使用示例

在任何 CoronaEngine 项目文件中：

```cpp
#include <compiler_detection.h>
#include <iostream>

int main() {
    std::cout << CORONA_COMPILE_INFO << std::endl;
    
    CORONA_PACKED_STRUCT(MyData) {
        char flag;
        int value;
    } CORONA_PACKED_END;
    
    std::cout << "MyData size: " << sizeof(MyData) << " bytes" << std::endl;
    return 0;
}
```

## 迁移指南

如果你之前使用了 `Utility/Concurrent` 中的旧版本 `compiler_detection.h`：

1. **包含路径更新**: 从 `#include "compiler_detection.h"` 改为 `#include <compiler_detection.h>`
2. **自动可用**: 不需要手动添加包含路径，系统已自动配置
3. **功能增强**: 新版本提供了更多实用宏和更好的跨平台支持

## 扩展开发

如需添加新的编译器或平台支持：

1. 在 `Common/include/compiler_detection.h` 中添加检测逻辑
2. 在 `Misc/cmake/CoronaCompileConfig.cmake` 中添加对应的 CMake 宏定义
3. 确保新功能在所有支持的平台上测试通过

## 测试验证

可以使用项目根目录下的 `test_global_compiler_detection.cpp` 来验证系统是否正常工作：

```bash
# 在 CoronaEngine 根目录下
cd test_cmake
cmake -B build -G Ninja
cmake --build build
./build/test_compiler_detection.exe
```

这将输出完整的编译器、平台和架构信息，以及紧密打包结构体的大小测试。