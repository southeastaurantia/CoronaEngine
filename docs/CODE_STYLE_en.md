# CoronaEngine Code Style Guide

This document describes the C++ code style conventions for the CoronaEngine project. We use clang-format and clang-tidy to automate code formatting and static analysis.

## Table of Contents

- [Core Principles](#core-principles)
- [Formatting Rules](#formatting-rules)
- [Naming Conventions](#naming-conventions)
- [Code Analysis Rules](#code-analysis-rules)
- [Tool Usage](#tool-usage)

## Core Principles

CoronaEngine's code style is based on the **Google C++ Style Guide** with some project-specific adjustments.

### Core Values

- **Consistency**: Maintain uniform style across the entire codebase
- **Readability**: Code should be easy to understand and maintain
- **Automation**: Use tools to enforce style rules, reducing human error

## Formatting Rules

We use the `.clang-format` file to define code formatting rules.

### Basic Configuration

```yaml
BasedOnStyle: Google
ColumnLimit: 0
IndentWidth: 4
UseTab: Never
```

### Detailed Explanation

#### Column Limit
- **No Limit**: `ColumnLimit: 0`
- No forced line breaks, allowing natural code organization based on logic
- Recommendation: Although there's no hard limit, maintain reasonable line lengths for readability

#### Indentation
- **Indent Width**: 4 spaces
- **No Tabs**: Always use spaces, never Tab characters
- Ensures consistent display across different editors

#### Base Style
- **Google Style**: Inherits most conventions from Google C++ Style Guide
- Includes brace placement, spacing, comment formatting, etc.

### Formatting Examples

```cpp
// Correct Example
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
        // Initialization code
    }

    TextureCache* texture_cache_;
    ShaderManager* shader_manager_;
};

}  // namespace Corona::Systems
```

## Naming Conventions

We use `.clang-tidy` to enforce naming conventions.

### Naming Convention Table

| Type | Convention | Examples |
|------|-----------|----------|
| **Class** | CamelCase | `RenderingSystem`, `TextureCache` |
| **Struct** | CamelCase | `VertexData`, `ShaderConfig` |
| **Interface** | CamelCase | `IRenderer`, `IResourceManager` |
| **Enum** | CamelCase | `RenderMode`, `TextureFormat` |
| **Function** | snake_case | `initialize()`, `get_texture()` |
| **Variable** | snake_case | `vertex_count`, `texture_id` |
| **Member Variable** | snake_case_ | `texture_cache_`, `render_queue_` |
| **Enum Constant** | kCamelCase | `kDefaultSize`, `kMaxTextures` |
| **Constant** | kCamelCase | `kPi`, `kMaxBufferSize` |
| **Global Constant** | kCamelCase | `kEngineVersion`, `kDefaultFPS` |

### Detailed Rules

#### Classes, Structs, Enums
```cpp
// Class names use CamelCase
class RenderingSystem { };
class TextureCache { };

// Struct names use CamelCase
struct VertexData {
    float x, y, z;
};

// Enum names use CamelCase
enum class RenderMode {
    kForward,    // Enum values use kCamelCase
    kDeferred,
    kRayTracing
};
```

#### Functions and Variables
```cpp
// Functions use snake_case
void initialize_renderer() { }
int calculate_frame_time() { }

// Local variables use snake_case
int vertex_count = 0;
float delta_time = 0.016f;
```

#### Member Variables
```cpp
class RenderingSystem {
  private:
    // Member variables end with underscore
    TextureCache* texture_cache_;
    int frame_counter_;
    bool is_initialized_;
};
```

#### Constants
```cpp
// Constants use kCamelCase prefix
constexpr int kMaxTextures = 256;
constexpr float kPi = 3.14159265359f;

class Config {
  public:
    static constexpr int kDefaultWidth = 1920;
    static constexpr int kDefaultHeight = 1080;
};
```

### Naming Best Practices

1. **Use Descriptive Names**: Names should clearly express intent
   ```cpp
   // Good
   int active_texture_count;
   
   // Bad
   int cnt;
   ```

2. **Avoid Abbreviations**: Unless widely recognized
   ```cpp
   // Good
   TextureManager tex_mgr;  // tex is a common abbreviation
   
   // Avoid
   int vtxCnt;  // Unclear abbreviation
   ```

3. **Boolean Variables**: Use clear prefixes
   ```cpp
   bool is_initialized_;
   bool has_alpha_channel_;
   bool should_update_;
   ```

## Code Analysis Rules

We use clang-tidy for static code analysis.

### Enabled Checks

```yaml
Checks: >
  google-*,
  -google-build-using-namespace,
  -google-readability-todo
```

### Check Descriptions

#### Enabled Google Checks
- **google-***: Enable all Google style checks
- Includes:
  - Readability checks
  - Performance checks
  - Modern C++ feature recommendations
  - Common error detection

#### Disabled Checks
1. **google-build-using-namespace**: Disabled
   - Reason: Allow `using namespace` in certain scenarios (e.g., inside cpp files)
   - Note: Still avoid in header files

2. **google-readability-todo**: Disabled
   - Reason: Allow flexible TODO comment formats
   - Support for mixed language TODO comments

### Common Check Items

#### 1. Modern C++ Features
```cpp
// Recommended: Use nullptr
Texture* texture = nullptr;

// Avoid: Using NULL or 0
Texture* texture = NULL;  // ❌
```

#### 2. Smart Pointer Usage
```cpp
// Recommended: Use smart pointers
std::unique_ptr<Renderer> renderer = std::make_unique<Renderer>();

// Avoid: Raw pointers and manual memory management
Renderer* renderer = new Renderer();  // ❌
```

#### 3. Range-based For Loops
```cpp
// Recommended: Use range-based for loops
for (const auto& texture : textures) {
    texture.bind();
}

// Avoid: Traditional index loops (in simple cases)
for (size_t i = 0; i < textures.size(); ++i) {  // Unnecessary
    textures[i].bind();
}
```

#### 4. Const Correctness
```cpp
// Recommended: Use const
void process_texture(const Texture& texture) {
    // Read-only operations
}

// Avoid: Unnecessary mutability
void process_texture(Texture& texture) {  // Shouldn't be this way if not modifying
    // Read-only operations
}
```

## Tool Usage

### clang-format

#### Format a Single File
```powershell
clang-format -i src/engine.cpp
```

#### Using Project Scripts
```powershell
# Format all staged files
.\code-format.ps1

# Check format (without modifying files)
.\code-format.ps1 -Check

# Format entire codebase
.\code-format.ps1 -All
```

### clang-tidy

#### Check a Single File
```powershell
clang-tidy src/engine.cpp -- -Ibuild/include
```

#### Using Project Scripts
```powershell
# Check all staged files
.\code-tidy.ps1

# Check entire codebase
.\code-tidy.ps1 -All
```

### Pre-commit Checks

Recommended to run before committing code:

```powershell
# 1. Format code
.\code-format.ps1

# 2. Run static analysis
.\code-tidy.ps1

# 3. Build project
cmake --build --preset msvc-debug

# 4. Run tests (if available)
ctest --preset msvc-debug
```

## Comment Style

### File Header Comments
```cpp
/**
 * @file rendering_system.h
 * @brief Core implementation of the rendering system
 * @author CoronaEngine Team
 * @date 2025-04-11
 */
```

### Class Comments
```cpp
/**
 * @brief RenderingSystem manages all rendering-related operations
 * 
 * RenderingSystem is responsible for coordinating the rendering pipeline,
 * managing rendering resources, processing draw calls, etc. It runs on 
 * a dedicated thread with priority 90.
 */
class RenderingSystem : public Kernel::SystemBase {
    // ...
};
```

### Function Comments
```cpp
/**
 * @brief Initialize the rendering system
 * @param ctx System context providing access to kernel services
 * @return Returns true if initialization succeeds, false otherwise
 */
bool initialize(ISystemContext* ctx) override;
```

### Inline Comments
```cpp
// Use concise English comments to explain "why", not "what"
texture_cache_.clear();  // Free unused textures to save memory

// Keep comments focused on non-obvious logic
int frame_count = 0;  // Current frame counter
```

## Namespaces

### Namespace Organization
```cpp
namespace Corona {
namespace Systems {

class RenderingSystem { };

}  // namespace Systems
}  // namespace Corona
```

### Namespace Closing Comments
- **Required**: Add comments when closing namespaces
- Format: `}  // namespace <NamespaceName>`

### Nested Namespaces (C++17)
```cpp
// Recommended: Use nested namespace syntax
namespace Corona::Systems {

class RenderingSystem { };

}  // namespace Corona::Systems
```

## Header Guards

Use `#pragma once`:

```cpp
#pragma once

namespace Corona {
// ...
}
```

## Include Order

```cpp
// 1. Corresponding header file (for .cpp files)
#include "corona/systems/rendering_system.h"

// 2. C system headers
#include <cstdint>
#include <cstring>

// 3. C++ standard library headers
#include <memory>
#include <string>
#include <vector>

// 4. Third-party library headers
#include <vulkan/vulkan.h>
#include <glfw/glfw3.h>

// 5. Project internal headers
#include "corona/engine.h"
#include "corona/events/rendering_events.h"
```

## Best Practices Summary

### ✅ Recommended Practices

1. **Use Automation Tools**: Rely on clang-format and clang-tidy, don't format manually
2. **Consistent Naming**: Strictly follow naming convention table
3. **Modern C++**: Use C++17/20 features (smart pointers, range loops, auto, etc.)
4. **Const Correctness**: Use const whenever possible
5. **Clear Comments**: Explain "why" rather than "what"
6. **Namespaces**: Use project namespaces to avoid conflicts

### ❌ Practices to Avoid

1. **Manual Formatting**: Don't manually adjust formatting, use tools
2. **Inconsistent Naming**: Mixing different naming styles
3. **Outdated C++ Features**: Avoid raw pointers, C-style casts
4. **Global Namespace Pollution**: Using `using namespace` in header files
5. **Meaningless Comments**: Commenting obvious code
6. **Magic Numbers**: Use named constants instead of hard-coded numbers

## Code Review Checklist

During code reviews, check the following items:

- [ ] Code has been formatted with clang-format
- [ ] Code passes clang-tidy checks without warnings
- [ ] Naming follows conventions (CamelCase for classes, snake_case for functions, etc.)
- [ ] Member variables end with underscore
- [ ] Constants use kCamelCase prefix
- [ ] Namespaces properly closed with comments
- [ ] Uses modern C++ features (smart pointers, auto, range loops, etc.)
- [ ] Appropriate const usage
- [ ] Clear and useful comments
- [ ] Correct header include order

## References

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [clang-format Documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
- [Modern C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices)

## Changelog

- **2025-04-11**: Initial version created based on project `.clang-format` and `.clang-tidy` configuration
