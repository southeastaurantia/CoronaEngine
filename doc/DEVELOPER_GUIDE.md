# CoronaEngine 开发者指南

CoronaEngine 项目开发规范、最佳实践和贡献指南

---

## 目录

- [项目概览](#项目概览)
- [开发环境配置](#开发环境配置)
- [代码规范](#代码规范)
- [命名约定](#命名约定)
- [项目结构](#项目结构)
- [构建系统](#构建系统)
- [Git 工作流](#git-工作流)
- [测试规范](#测试规范)
- [文档规范](#文档规范)
- [性能优化指南](#性能优化指南)
- [常见错误与解决方案](#常见错误与解决方案)

---

## 项目概览

### 技术栈

- **核心语言**: C++20
- **构建系统**: CMake 4.0+
- **编译器**: Clang (默认), MSVC, GCC
- **脚本语言**: Python 3.13+
- **图形 API**: Vulkan (通过 Helicon)
- **ECS 框架**: EnTT
- **并发库**: Intel TBB 2022.2.0

### 架构特点

- **模块化设计**: 核心引擎 + 插件式系统
- **静态链接库**: 主引擎作为静态库提供
- **独立工具库**: Utility 模块可独立使用
- **热重载支持**: Python 脚本实时热重载
- **跨平台**: Windows / Linux / macOS

---

## 开发环境配置

### 必需工具

#### Windows

```powershell
# 1. LLVM Clang (推荐)
# 下载: https://github.com/llvm/llvm-project/releases
# 或使用 Visual Studio Clang 工具链

# 2. CMake 4.0+
choco install cmake --version=4.0.0

# 3. Ninja
choco install ninja

# 4. Python 3.13+
choco install python --version=3.13.0

# 5. Git
choco install git
```

#### Linux (Ubuntu/Debian)

```bash
# 1. Clang
sudo apt install clang-14 lldb-14 lld-14

# 2. CMake
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install cmake

# 3. Ninja
sudo apt install ninja-build

# 4. Python 3.13+
sudo apt install python3.13 python3.13-dev

# 5. 开发工具
sudo apt install build-essential git
```

#### macOS

```bash
# 1. Xcode Command Line Tools
xcode-select --install

# 2. Homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 3. 开发工具
brew install cmake ninja python@3.13 llvm
```

### 推荐 IDE/编辑器

#### Visual Studio Code

**推荐扩展**:
```json
{
  "recommendations": [
    "ms-vscode.cpptools",
    "ms-vscode.cmake-tools",
    "llvm-vs-code-extensions.vscode-clangd",
    "xaver.clang-format",
    "notskm.clang-tidy",
    "ms-python.python",
    "github.copilot"
  ]
}
```

**配置 clangd** (`.vscode/settings.json`):
```json
{
  "clangd.arguments": [
    "--compile-commands-dir=${workspaceFolder}/build",
    "--background-index",
    "--clang-tidy",
    "--header-insertion=iwyu"
  ],
  "C_Cpp.intelliSenseEngine": "Disabled"
}
```

#### Visual Studio 2022

使用 `vs2022` 预设：
```powershell
cmake --preset vs2022
# 打开生成的 build/CoronaEngine.sln
```

---

## 代码规范

### C++ 标准

- **C++20**: 必须使用 C++20 标准
- **C++23**: 可选特性，需添加编译器检查

```cpp
#include <compiler_features.h>

#if CE_BUILTIN_CPP20
    // C++20 特性
    #include <concepts>
    #include <ranges>
#endif
```

### 编译器特性宏

使用 `compiler_features.h` 提供的跨平台宏，不要直接使用编译器特定语法。

**❌ 错误**:
```cpp
#ifdef _MSC_VER
    __forceinline int fast_add(int a, int b) { return a + b; }
#else
    inline __attribute__((always_inline)) int fast_add(int a, int b) { return a + b; }
#endif
```

**✅ 正确**:
```cpp
#include <compiler_features.h>

CE_BUILTIN_FORCE_INLINE int fast_add(int a, int b) {
    return a + b;
}
```

### 代码风格

项目使用 **Google C++ Style Guide** 作为基础，通过 **Clang-Tidy** 和 **Clang-Format** 强制执行。

#### Clang-Tidy 检查

启用规则 (`.clang-tidy`):
- `google-*`: Google 风格指南
- 禁用: `google-build-using-namespace`, `google-readability-todo`

#### Clang-Format 配置

```bash
# 格式化单个文件
clang-format -i src/core/engine/Engine.cpp

# 格式化整个目录
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### 头文件包含顺序

**标准顺序**:
1. 对应的头文件（如果是 .cpp）
2. C 标准库
3. C++ 标准库
4. 第三方库
5. 项目公共头文件
6. 项目内部头文件

**示例**:
```cpp
// MyClass.cpp
#include "MyClass.h"          // 1. 对应头文件

#include <cstdio>             // 2. C 标准库
#include <cstring>

#include <iostream>           // 3. C++ 标准库
#include <memory>
#include <vector>

#include <entt/entt.hpp>      // 4. 第三方库
#include <spdlog/spdlog.h>

#include <compiler_features.h> // 5. 项目公共头文件
#include <Log.h>

#include "Core/Engine/Engine.h" // 6. 项目内部头文件
```

### 头文件保护

使用 `#pragma once`（简洁且无命名冲突）。

**✅ 推荐**:
```cpp
#pragma once

namespace Corona {
    class MyClass { };
}
```

**❌ 不推荐**:
```cpp
#ifndef CORONA_MY_CLASS_H_
#define CORONA_MY_CLASS_H_

namespace Corona {
    class MyClass { };
}

#endif  // CORONA_MY_CLASS_H_
```

### 注释规范

#### 文件头注释

```cpp
/**
 * @file engine.h
 * @brief CoronaEngine 核心引擎类定义
 * @author CoronaEngine Team
 * @date 2025-10-05
 */
```

#### 类/函数注释

```cpp
/**
 * @brief 引擎主类，管理所有系统和资源
 * 
 * Engine 是单例类，负责：
 * - 系统注册与生命周期管理
 * - 资源管理器访问
 * - 线程安全的命令队列
 * 
 * @note 线程安全：所有公共方法均线程安全
 * @see ISystem, ResourceManager
 */
class Engine {
public:
    /**
     * @brief 初始化引擎
     * @param cfg 日志配置
     * @throw std::runtime_error 如果初始化失败
     */
    void Init(const LogConfig& cfg);
};
```

#### 行内注释

```cpp
// 使用 TBB 的并发哈希表以提高多线程性能
tbb::concurrent_hash_map<ResourceId, ResourcePtr> cache_;

int compute() {
    // TODO(username): 优化此算法的时间复杂度
    // FIXME(username): 处理边界情况 n=0
    return slow_algorithm();
}
```

### 错误处理

#### 使用异常

```cpp
// ✅ 构造函数、资源加载等不可恢复错误
ResourceManager::ResourceManager() {
    if (!initialize_internal()) {
        throw std::runtime_error("ResourceManager initialization failed");
    }
}

// ✅ 使用标准异常类型
void load_config(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw std::invalid_argument("Config file not found: " + path);
    }
}
```

#### 使用返回值

```cpp
// ✅ 可恢复错误、性能关键路径
std::optional<Model> load_model(const std::string& path) {
    if (!validate_path(path)) {
        Logger::Warn("Invalid model path: {}", path);
        return std::nullopt;
    }
    // ...
}

// ✅ 使用 expected (C++23)
#if CE_BUILTIN_CPP23
std::expected<Model, LoadError> load_model_checked(const std::string& path);
#endif
```

#### 日志记录

```cpp
#include <Log.h>

void process_data() {
    Logger::Debug("Processing started");
    
    if (CE_BUILTIN_UNLIKELY(data == nullptr)) {
        Logger::Error("Null data pointer");
        return;
    }
    
    try {
        risky_operation();
    } catch (const std::exception& e) {
        Logger::Critical("Fatal error: {}", e.what());
        throw;  // 重新抛出
    }
}
```

---

## 命名约定

### C++ 代码命名

遵循 `.clang-tidy` 配置的命名规则：

| 类型 | 风格 | 示例 | 说明 |
|------|------|------|------|
| **命名空间** | `CamelCase` | `Corona`, `Corona::Rendering` | 每个模块一个命名空间 |
| **类/结构体** | `CamelCase` | `Engine`, `ResourceManager` | 类型名使用大驼峰 |
| **接口** | `I` + `CamelCase` | `ISystem`, `IResource` | 接口前缀 `I` |
| **枚举** | `CamelCase` | `LogLevel`, `RenderPass` | 枚举名大驼峰 |
| **枚举常量** | `k` + `CamelCase` | `kMaxSize`, `kDefaultValue` | 前缀 `k` |
| **全局常量** | `k` + `CamelCase` | `kPi`, `kEpsilon` | 前缀 `k` |
| **函数** | `snake_case` | `load_model()`, `start_system()` | 全小写+下划线 |
| **变量** | `snake_case` | `model_path`, `frame_count` | 全小写+下划线 |
| **成员变量** | `snake_case_` | `resource_cache_`, `mutex_` | 后缀下划线 |
| **私有成员** | `snake_case_` | `impl_`, `internal_state_` | 后缀下划线 |
| **模板参数** | `CamelCase` | `T`, `Container`, `Allocator` | 类型参数大驼峰 |

**示例代码**:
```cpp
namespace Corona {

// 常量
constexpr int kMaxThreads = 16;
constexpr float kPi = 3.14159f;

// 枚举
enum class RenderMode {
    kForward,
    kDeferred,
    kRayTracing
};

// 接口
class ISystem {
public:
    virtual void start() = 0;
    virtual void stop() = 0;
};

// 类
class RenderingSystem : public ISystem {
public:
    void start() override;
    void load_shader(const std::string& path);
    
    int get_frame_count() const { return frame_count_; }

private:
    void update_internal();
    
    int frame_count_ = 0;           // 成员变量
    std::mutex mutex_;              // 成员变量
    std::unique_ptr<Impl> impl_;    // 私有实现
};

// 函数
std::shared_ptr<Model> load_model(const ResourceId& id);

// 模板
template<typename T, typename Allocator = std::allocator<T>>
class Container {
    // ...
};

}  // namespace Corona
```

### 文件和文件夹命名

**规则**: 使用 `snake_case`

| 类型 | 风格 | 示例 |
|------|------|------|
| **源文件** | `snake_case.cpp` | `rendering_system.cpp`, `model_loader.cpp` |
| **头文件** | `snake_case.h` | `rendering_system.h`, `model_loader.h` |
| **目录** | `snake_case/` | `resource_manager/`, `python_scripting/` |

**例外情况** (保持原始命名):
- `README.md`, `LICENSE`, `AUTHORS`
- `CMakeLists.txt`, `CMakePresets.json`
- `.clang-tidy`, `.clang-format`, `.gitignore`, `.clang-format`
- 主要文档: `CMAKE_BUILD_GUIDE.md` (UPPER_SNAKE_CASE)
- 第三方代码: 保持上游项目原始命名

**目录结构示例**:
```
src/
├── core/
│   ├── engine/
│   │   ├── Engine.h
│   │   ├── Engine.cpp
│   │   ├── ISystem.h
│   │   ├── ThreadedSystem.h
│   │   └── systems/
│   │       ├── AnimationSystem.h
│   │       ├── AudioSystem.h
│   │       ├── DisplaySystem.h
│   │       └── RenderingSystem.h
│   └── thread/
│       ├── SafeCommandQueue.h
│       └── SafeDataCache.h
└── utility/
    ├── logger/
    │   ├── include/
    │   │   └── Log.h
    │   └── Log.cpp
    └── resource_manager/
        ├── include/
        │   └── ResourceManager.h
        └── ResourceManager.cpp
```

### CMake 命名

| 类型 | 风格 | 示例 |
|------|------|------|
| **变量** | `UPPER_SNAKE_CASE` | `CORONA_BUILD_EXAMPLES` |
| **函数** | `snake_case` | `corona_add_utility()` |
| **目标** | `CamelCase` | `CoronaEngine`, `CoronaLogger` |
| **别名** | `Corona::` + `CamelCase` | `Corona::Engine`, `Corona::Logger` |
| **选项** | `UPPER_SNAKE_CASE` | `BUILD_SHARED_LIBS` |

**示例**:
```cmake
# 选项
option(CORONA_BUILD_EXAMPLES "Build examples" ON)

# 变量
set(CORONA_VERSION "0.5.0")

# 函数
function(corona_add_utility)
    # ...
endfunction()

# 目标
add_library(CoronaLogger STATIC ${SOURCES})

# 别名
add_library(Corona::Logger ALIAS CoronaLogger)
```

---

## 项目结构

### 核心目录

```
CoronaEngine/
├── .github/                    # GitHub 配置（copilot-instructions.md 为本地配置，不提交）
├── build/                      # 构建输出目录（.gitignore）
├── doc/                        # 项目文档
│   ├── CMAKE_BUILD_GUIDE.md
│   ├── COMPILER_FEATURES.md
│   └── DEVELOPER_GUIDE.md
├── editor/                     # 编辑器
│   └── CoronaEditor/
│       ├── Backend/            # PyQt6 后端
│       └── Frontend/           # Vue.js 前端
├── examples/                   # 示例程序
│   ├── assets/                 # 共享资源
│   ├── concurrent_performance/
│   ├── interactive_rendering/
│   ├── multi_window_rendering/
│   ├── python_scripting/
│   └── resource_management/
├── misc/                       # 辅助脚本
│   ├── cmake/                  # CMake 模块
│   └── pytools/                # Python 工具
├── src/                        # 源代码
│   ├── common/                 # 公共头文件
│   │   └── include/
│   │       └── compiler_features.h
│   ├── core/                   # 核心引擎
│   │   ├── engine/
│   │   └── thread/
│   ├── resource/               # 资源类型
│   ├── script/                 # 脚本系统
│   │   └── python/
│   └── utility/                # 独立工具库
│       ├── logger/
│       ├── resource_manager/
│       └── concurrent/
├── third_party/                # 第三方库
│   ├── oneapi-tbb-2022.2.0/
│   ├── Python-3.13.7/
│   └── spdlog-1.15.3/
├── .clang-format               # 代码格式化配置
├── .clang-tidy                 # 代码检查配置
├── .gitignore
├── CMakeLists.txt              # 根 CMake 文件
├── CMakePresets.json           # CMake 预设
└── README.md
```

### 模块职责

#### `src/core/` - 核心引擎

- **engine/**: 引擎主类、系统接口
- **thread/**: 线程安全工具（命令队列、数据缓存）

#### `src/resource/` - 资源管理

- 资源类型定义（Model, Mesh, Shader 等）
- Assimp 集成

#### `src/script/` - 脚本系统

- **python/**: Python C API 集成
- 热重载实现

#### `src/utility/` - 独立工具库

每个 utility 是独立的静态库：
- **logger**: 日志系统（spdlog 封装）
- **resource_manager**: 资源管理器
- **concurrent**: 并发工具（TBB 集成）

---

## 构建系统

详细信息请参阅 [CMAKE_BUILD_GUIDE.md](CMAKE_BUILD_GUIDE.md)。

### 快速参考

#### 添加新的 Utility 模块

1. 创建目录: `src/utility/my_module/`
2. 添加 `CMakeLists.txt`:
   ```cmake
   corona_add_utility(
       NAME CoronaMyModule
       TYPE STATIC
       SOURCES my_impl.cpp
       PUBLIC_HEADERS include/MyHeader.h
   )
   ```
3. 注册到 `src/utility/CMakeLists.txt`

#### 添加新的示例

1. 创建目录: `examples/my_example/`
2. 添加 `CMakeLists.txt`:
   ```cmake
   corona_add_example(
       NAME Corona_my_example
       SOURCES my_example.cpp
       COPY_ASSETS
   )
   ```
3. 注册到 `examples/CMakeLists.txt`

---

## Git 工作流

### 分支策略

- **main**: 主分支，始终保持稳定
- **develop**: 开发分支，日常开发
- **feature/xxx**: 功能分支
- **bugfix/xxx**: 修复分支
- **release/x.x.x**: 发布分支

### Commit 规范

使用 [Conventional Commits](https://www.conventionalcommits.org/) 格式：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type**:
- `feat`: 新功能
- `fix`: 修复
- `docs`: 文档
- `style`: 格式（不影响代码运行）
- `refactor`: 重构
- `perf`: 性能优化
- `test`: 测试
- `chore`: 构建/工具

**示例**:
```
feat(rendering): add deferred rendering pipeline

Implement deferred rendering with G-buffer:
- Position buffer
- Normal buffer
- Albedo buffer
- Metallic-Roughness buffer

Closes #123
```

```
fix(resource): prevent memory leak in model loader

The model loader was not releasing GPU resources properly
when loading failed. Added proper cleanup in destructor.

Fixes #456
```

### Pull Request 流程

1. **创建分支**:
   ```bash
   git checkout -b feature/my-feature develop
   ```

2. **开发并提交**:
   ```bash
   git add .
   git commit -m "feat(module): add new feature"
   ```

3. **推送并创建 PR**:
   ```bash
   git push origin feature/my-feature
   ```

4. **代码审查**: 至少一位维护者审批

5. **合并**: Squash and merge 到 develop

---

## 测试规范

### 单元测试

**计划中** - 目前项目尚未集成测试框架

预期使用：
- **Google Test** (gtest/gmock)
- **Catch2**

### 示例作为集成测试

当前使用 `examples/` 目录中的示例程序作为集成测试：

```powershell
# 构建所有示例
cmake --build build --config Debug

# 运行特定示例
.\build\examples\interactive_rendering\Debug\Corona_interactive_rendering.exe
```

---

## 文档规范

### Markdown 文档

- 使用 `snake_case` 或 `UPPER_SNAKE_CASE`
- 包含目录
- 代码块指定语言
- 使用相对链接

**示例**:
```markdown
# 文档标题

## 目录

- [章节 1](#章节-1)
- [章节 2](#章节-2)

## 章节 1

内容...

## 章节 2

内容...
```

### 代码文档

使用 Doxygen 风格注释：

```cpp
/**
 * @brief 简要描述
 * 
 * 详细描述...
 * 
 * @param param1 参数 1 描述
 * @param param2 参数 2 描述
 * @return 返回值描述
 * @throw std::exception 异常描述
 * @note 注意事项
 * @warning 警告信息
 * @see 相关类/函数
 * @example
 * @code
 * int result = my_function(1, 2);
 * @endcode
 */
int my_function(int param1, int param2);
```

---

## 性能优化指南

### 编译时优化

#### 使用编译器特性宏

```cpp
#include <compiler_features.h>

// 热路径函数
CE_BUILTIN_HOT CE_BUILTIN_FORCE_INLINE
int critical_function() {
    // ...
}

// 冷路径函数（错误处理）
CE_BUILTIN_COLD
void error_handler() {
    // ...
}

// 分支预测
if (CE_BUILTIN_LIKELY(condition)) {
    // 常见情况
} else {
    // 罕见情况
}
```

#### 对齐优化

```cpp
// 缓存行对齐（避免 false sharing）
CE_BUILTIN_ALIGN(64) struct CacheLineAligned {
    std::atomic<int> counter;
};

// SIMD 对齐
CE_BUILTIN_ALIGN(16) float vec4[4];
```

### 运行时优化

#### 使用对象池

```cpp
#include <concurrent.h>

// TBB 对象池
tbb::concurrent_bounded_queue<Model*> model_pool;

Model* acquire_model() {
    Model* model;
    if (model_pool.try_pop(model)) {
        model->reset();
        return model;
    }
    return new Model();
}

void release_model(Model* model) {
    model_pool.push(model);
}
```

#### 并发容器

```cpp
#include <concurrent.h>

// 使用 TBB 并发容器
tbb::concurrent_hash_map<ResourceId, ResourcePtr> cache_;
tbb::concurrent_vector<Task> tasks_;
```

#### 避免内存分配

```cpp
// ❌ 避免：每帧分配
void update() {
    std::vector<Entity> entities = get_entities();  // 分配
    for (auto& e : entities) {
        // ...
    }
}

// ✅ 推荐：复用容器
class System {
    std::vector<Entity> entities_;  // 成员变量
    
    void update() {
        entities_.clear();  // 不释放内存
        get_entities(entities_);  // 填充已有容器
        for (auto& e : entities_) {
            // ...
        }
    }
};
```

### 性能测量

```cpp
#include <chrono>

// 简单计时
auto start = std::chrono::high_resolution_clock::now();
expensive_operation();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
Logger::Info("Operation took {} ms", duration.count());

// RAII 计时器
class ScopedTimer {
public:
    ScopedTimer(const char* name) : name_(name), start_(std::chrono::high_resolution_clock::now()) {}
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
        Logger::Debug("{} took {} ms", name_, ms);
    }
private:
    const char* name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

void function() {
    ScopedTimer timer("function");
    // ...
}
```

---

## 常见错误与解决方案

### 编译错误

#### 错误 1: 找不到头文件

**错误信息**:
```
fatal error: Log.h: No such file or directory
```

**原因**: 包含路径配置错误

**解决方案**:
```cpp
// ❌ 错误
#include "Log.h"

// ✅ 正确 - Utility 模块
#include <Log.h>

// ✅ 正确 - 核心模块
#include <Core/Engine/Engine.h>
```

#### 错误 2: 链接错误

**错误信息**:
```
undefined reference to `Corona::Logger::Init()'
```

**原因**: 未链接所需库

**解决方案**:
```cmake
target_link_libraries(MyTarget PRIVATE
    CoronaEngine
    Corona::Logger  # 添加依赖
)
```

### 运行时错误

#### 错误 1: DLL 找不到

**错误信息**:
```
The code execution cannot proceed because tbb12.dll was not found.
```

**解决方案**:
```cmake
# 确保调用了运行时依赖复制
corona_install_runtime_deps(MyTarget)
```

#### 错误 2: Python 模块找不到

**错误信息**:
```
ModuleNotFoundError: No module named 'xxx'
```

**解决方案**:
```powershell
# 安装缺失的 Python 包
python -m pip install -r misc\pytools\requirements.txt

# 或启用自动安装（重新配置）
cmake --preset ninja-clang -DCORONA_AUTO_INSTALL_PY_DEPS=ON
```

### 代码规范错误

#### 错误 1: Clang-Tidy 警告

**警告信息**:
```
warning: invalid case style for function 'LoadModel' [readability-identifier-naming]
```

**解决方案**:
```cpp
// ❌ 错误
void LoadModel();

// ✅ 正确
void load_model();
```

#### 错误 2: 未使用的变量

**警告信息**:
```
warning: unused variable 'result' [-Wunused-variable]
```

**解决方案**:
```cpp
// 方案 1: 删除未使用的变量
// int result = calculate();

// 方案 2: 标记为可能未使用
CE_BUILTIN_MAYBE_UNUSED int result = calculate();

// 方案 3: 使用 [[maybe_unused]] (C++17)
[[maybe_unused]] int result = calculate();
```

---

## 贡献指南

### 如何贡献

1. **Fork 仓库**
2. **创建功能分支**
3. **编写代码** (遵循本文档规范)
4. **添加测试** (如果适用)
5. **更新文档**
6. **提交 Pull Request**

### Pull Request 清单

- [ ] 代码遵循命名约定
- [ ] 通过 Clang-Tidy 检查
- [ ] 通过 Clang-Format 格式化
- [ ] 添加必要的注释
- [ ] 更新相关文档
- [ ] 通过所有示例编译
- [ ] Commit 信息符合规范

### 代码审查

审查者关注点：
- **功能正确性**: 代码是否实现预期功能
- **性能**: 是否有明显的性能问题
- **可维护性**: 代码是否易于理解和修改
- **规范遵循**: 是否符合项目规范
- **安全性**: 是否有安全隐患

---

## 参考资源

### 外部文档

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [CMake Documentation](https://cmake.org/documentation/)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [EnTT Documentation](https://github.com/skypjack/entt/wiki)

### 项目文档

- [CMAKE_BUILD_GUIDE.md](CMAKE_BUILD_GUIDE.md) - CMake 构建系统指南
- [COMPILER_FEATURES.md](COMPILER_FEATURES.md) - 编译器特性宏文档

### 工具

- **Clang-Tidy**: `clang-tidy --checks='*' src/core/engine/Engine.cpp`
- **Clang-Format**: `clang-format -i src/core/engine/Engine.cpp`
- **CMake**: `cmake --help`
- **Python**: `python --version`

---

## 附录

### 开发环境变量

```bash
# Windows (PowerShell)
$env:LLVM_PATH = "C:\Program Files\LLVM"
$env:CMAKE_GENERATOR = "Ninja Multi-Config"

# Linux / macOS (Bash)
export LLVM_PATH="/usr/lib/llvm-14"
export CMAKE_GENERATOR="Ninja Multi-Config"
```

### 编辑器配置

#### .editorconfig

```ini
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true

[*.{cpp,h,hpp}]
indent_style = space
indent_size = 4

[*.cmake]
indent_style = space
indent_size = 4

[*.py]
indent_style = space
indent_size = 4

[*.{json,yml,yaml}]
indent_style = space
indent_size = 2
```

---

**文档版本**: 1.0  
**最后更新**: 2025-10-05  
**引擎版本**: CoronaEngine 0.5.0  
**维护者**: CoronaEngine Team

---

**欢迎贡献！** 如有任何问题或建议，请提交 Issue 或 Pull Request。
