# CoronaEngine 代码风格指南

本文档描述了 CoronaEngine 项目的 C++ 代码风格规范。我们使用 clang-format 和 clang-tidy 来自动化代码格式化和静态分析。

## 目录

- [基础原则](#基础原则)
- [格式化规则](#格式化规则)
- [命名规范](#命名规范)
- [代码检查规则](#代码检查规则)
- [工具使用](#工具使用)

## 基础原则

CoronaEngine 的代码风格基于 **Google C++ Style Guide**，并针对项目需求进行了部分调整。

### 核心价值观

- **一致性**: 整个代码库保持统一的风格
- **可读性**: 代码应该易于理解和维护
- **自动化**: 使用工具强制执行风格规则，减少人为错误

## 格式化规则

我们使用 `.clang-format` 文件定义代码格式化规则。

### 基本配置

```yaml
BasedOnStyle: Google
ColumnLimit: 0
IndentWidth: 4
UseTab: Never
```

### 详细说明

#### 行宽限制
- **无限制**: `ColumnLimit: 0`
- 不强制换行，允许根据代码逻辑自然组织
- 建议：虽然没有硬性限制，但仍应保持合理的行长度以提高可读性

#### 缩进
- **缩进宽度**: 4 个空格
- **禁用制表符**: 始终使用空格，不使用 Tab 字符
- 这确保了代码在不同编辑器中显示一致

#### 基础风格
- **Google 风格**: 继承 Google C++ Style Guide 的大部分约定
- 包括大括号位置、空格使用、注释格式等

### 格式化示例

```cpp
// 正确示例
namespace Corona::Systems {

class RenderingSystem : public Kernel::SystemBase {
  public:
    RenderingSystem() : texture_cache_(nullptr), shader_manager_(nullptr) {
        initialize_resources();
    }

    void update() override {
        process_render_queue();
        submit_command_buffers();
    }

  private:
    void initialize_resources() {
        // 初始化代码
    }

    TextureCache* texture_cache_;
    ShaderManager* shader_manager_;
};

}  // namespace Corona::Systems
```

## 命名规范

我们使用 `.clang-tidy` 强制执行命名规范。

### 命名约定表

| 类型 | 约定 | 示例 |
|------|------|------|
| **类 (Class)** | CamelCase | `RenderingSystem`, `TextureCache` |
| **结构体 (Struct)** | CamelCase | `VertexData`, `ShaderConfig` |
| **接口 (Interface)** | CamelCase | `IRenderer`, `IResourceManager` |
| **枚举 (Enum)** | CamelCase | `RenderMode`, `TextureFormat` |
| **函数 (Function)** | snake_case | `initialize()`, `get_texture()` |
| **变量 (Variable)** | snake_case | `vertex_count`, `texture_id` |
| **成员变量 (Member)** | snake_case_ | `texture_cache_`, `render_queue_` |
| **枚举常量 (Enum Constant)** | kCamelCase | `kDefaultSize`, `kMaxTextures` |
| **常量 (Constant)** | kCamelCase | `kPi`, `kMaxBufferSize` |
| **全局常量 (Global Constant)** | kCamelCase | `kEngineVersion`, `kDefaultFPS` |

### 详细规则

#### 类、结构体、枚举
```cpp
// 类名使用 CamelCase
class RenderingSystem { };
class TextureCache { };

// 结构体使用 CamelCase
struct VertexData {
    float x, y, z;
};

// 枚举使用 CamelCase
enum class RenderMode {
    kForward,    // 枚举值使用 kCamelCase
    kDeferred,
    kRayTracing
};
```

#### 函数和变量
```cpp
// 函数使用 snake_case
void initialize_renderer() { }
int calculate_frame_time() { }

// 局部变量使用 snake_case
int vertex_count = 0;
float delta_time = 0.016f;
```

#### 成员变量
```cpp
class RenderingSystem {
  private:
    // 成员变量以下划线结尾
    TextureCache* texture_cache_;
    int frame_counter_;
    bool is_initialized_;
};
```

#### 常量
```cpp
// 常量使用 kCamelCase 前缀
constexpr int kMaxTextures = 256;
constexpr float kPi = 3.14159265359f;

class Config {
  public:
    static constexpr int kDefaultWidth = 1920;
    static constexpr int kDefaultHeight = 1080;
};
```

### 命名最佳实践

1. **使用描述性名称**: 名称应清楚表达意图
   ```cpp
   // 好
   int active_texture_count;
   
   // 不好
   int cnt;
   ```

2. **避免缩写**: 除非是广为人知的缩写
   ```cpp
   // 好
   TextureManager tex_mgr;  // tex 是常见缩写
   
   // 避免
   int vtxCnt;  // 不清晰的缩写
   ```

3. **布尔变量**: 使用明确的前缀
   ```cpp
   bool is_initialized_;
   bool has_alpha_channel_;
   bool should_update_;
   ```

## 代码检查规则

我们使用 clang-tidy 进行静态代码分析。

### 启用的检查

```yaml
Checks: >
  google-*,
  -google-build-using-namespace,
  -google-readability-todo
```

### 检查说明

#### 启用的 Google 检查
- **google-***: 启用所有 Google 风格检查
- 包括：
  - 可读性检查
  - 性能检查
  - 现代化 C++ 特性推荐
  - 常见错误检测

#### 禁用的检查
1. **google-build-using-namespace**: 禁用
   - 理由：允许在某些场景下使用 `using namespace`（如 cpp 文件内部）
   - 注意：头文件中仍应避免使用

2. **google-readability-todo**: 禁用
   - 理由：允许灵活的 TODO 注释格式
   - 支持中英文混合的 TODO 注释

### 常见检查项

#### 1. 现代 C++ 特性
```cpp
// 推荐：使用 nullptr
Texture* texture = nullptr;

// 避免：使用 NULL 或 0
Texture* texture = NULL;  // ❌
```

#### 2. 智能指针使用
```cpp
// 推荐：使用智能指针
std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>();

// 避免：裸指针和手动内存管理
Renderer* renderer = new Renderer();  // ❌
```

#### 3. 范围循环
```cpp
// 推荐：使用范围 for 循环
for (const auto& texture : textures) {
    texture.bind();
}

// 避免：传统索引循环（在简单情况下）
for (size_t i = 0; i < textures.size(); ++i) {  // 不必要
    textures[i].bind();
}
```

#### 4. 常量正确性
```cpp
// 推荐：使用 const
void process_texture(const Texture& texture) {
    // 只读操作
}

// 避免：不必要的可变性
void process_texture(Texture& texture) {  // 如果不修改则不应该这样
    // 只读操作
}
```

## 工具使用

### clang-format

#### 格式化单个文件
```powershell
clang-format -i src/engine.cpp
```

#### 使用项目脚本
```powershell
# 格式化所有暂存的文件
.\code-format.ps1

# 检查格式（不修改文件）
.\code-format.ps1 -Check

# 格式化整个代码库
.\code-format.ps1 -All
```

### clang-tidy

#### 检查单个文件
```powershell
clang-tidy src/engine.cpp -- -Ibuild/include
```

#### 使用项目脚本
```powershell
# 检查所有暂存的文件
.\code-tidy.ps1

# 检查整个代码库
.\code-tidy.ps1 -All
```

### 预提交检查

建议在提交代码前运行：

```powershell
# 1. 格式化代码
.\code-format.ps1

# 2. 运行静态分析
.\code-tidy.ps1

# 3. 构建项目
cmake --build --preset msvc-debug

# 4. 运行测试（如果有）
ctest --preset msvc-debug
```

## 注释风格

### 文件头注释
```cpp
/**
 * @file rendering_system.h
 * @brief 渲染系统的核心实现
 * @author CoronaEngine Team
 * @date 2025-04-11
 */
```

### 类注释
```cpp
/**
 * @brief 渲染系统管理所有渲染相关操作
 * 
 * RenderingSystem 负责协调渲染管线、管理渲染资源、
 * 处理绘制调用等。它运行在独立线程上，优先级为 90。
 */
class RenderingSystem : public Kernel::SystemBase {
    // ...
};
```

### 函数注释
```cpp
/**
 * @brief 初始化渲染系统
 * @param ctx 系统上下文，提供对内核服务的访问
 * @return 初始化成功返回 true，否则返回 false
 */
bool initialize(ISystemContext* ctx) override;
```

### 行内注释
```cpp
// 使用简洁的英文注释说明"为什么"，而不是"是什么"
texture_cache_.clear();  // 释放未使用的纹理以节省内存

// 中文注释也可以接受，但公共 API 优先使用英文
int frame_count = 0;  // 当前帧计数器
```

## 命名空间

### 命名空间组织
```cpp
namespace Corona {
namespace Systems {

class RenderingSystem { };

}  // namespace Systems
}  // namespace Corona
```

### 命名空间关闭注释
- **必须**: 在命名空间结束时添加注释
- 格式: `}  // namespace <NamespaceName>`

### 嵌套命名空间（C++17）
```cpp
// 推荐：使用嵌套命名空间语法
namespace Corona::Systems {

class RenderingSystem { };

}  // namespace Corona::Systems
```

## 头文件保护

使用 `#pragma once`:

```cpp
#pragma once

namespace Corona {
// ...
}
```

## 包含顺序

```cpp
// 1. 对应的头文件（对于 .cpp 文件）
#include "corona/systems/rendering_system.h"

// 2. C 系统头文件
#include <cstdint>
#include <cstring>

// 3. C++ 标准库头文件
#include <memory>
#include <string>
#include <vector>

// 4. 第三方库头文件
#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>

// 5. 项目内部头文件
#include "corona/engine.h"
#include "corona/events/rendering_events.h"
```

## 最佳实践总结

### ✅ 推荐做法

1. **使用自动化工具**: 依赖 clang-format 和 clang-tidy，不手动格式化
2. **一致的命名**: 严格遵循命名规范表
3. **现代 C++**: 使用 C++17/20 特性（智能指针、范围循环、auto 等）
4. **常量正确性**: 尽可能使用 const
5. **清晰的注释**: 解释"为什么"而不是"是什么"
6. **命名空间**: 使用项目命名空间避免冲突

### ❌ 避免做法

1. **手动格式化**: 不要手动调整格式，使用工具
2. **不一致的命名**: 混合使用不同的命名风格
3. **过时的 C++ 特性**: 避免使用裸指针、C 风格类型转换
4. **全局命名空间污染**: 头文件中使用 `using namespace`
5. **无意义的注释**: 注释显而易见的代码
6. **魔法数字**: 使用命名常量替代硬编码的数字

## 代码审查检查清单

在代码审查时，检查以下项目：

- [ ] 代码已通过 clang-format 格式化
- [ ] 代码已通过 clang-tidy 检查无警告
- [ ] 命名遵循规范（类用 CamelCase，函数用 snake_case 等）
- [ ] 成员变量以下划线结尾
- [ ] 常量使用 kCamelCase 前缀
- [ ] 命名空间正确关闭并添加注释
- [ ] 使用现代 C++ 特性（智能指针、auto、范围循环等）
- [ ] 适当的 const 使用
- [ ] 清晰有用的注释
- [ ] 头文件包含顺序正确

## 参考资源

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [clang-format 文档](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy 文档](https://clang.llvm.org/extra/clang-tidy/)
- [Modern C++ 最佳实践](https://github.com/cpp-best-practices/cppbestpractices)

## 更新日志

- **2025-04-11**: 初始版本，基于项目 `.clang-format` 和 `.clang-tidy` 配置创建
