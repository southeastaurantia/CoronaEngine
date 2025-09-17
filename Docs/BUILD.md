# CoronaEngine 构建与预设指南

> 本文档全面说明 CoronaEngine 的 CMake 构建体系：目录结构、模块职责、构建/运行策略、`CMakePresets.json` 预设使用、Python 与运行时依赖处理、第三方管理、以及常见问题排查与扩展建议。

---
## 目录
1. 环境 / 前置依赖
2. 仓库目录结构速览
3. 构建模块分层 (Corona*.cmake) 概览
4. CMake 选项与核心变量
5. 预设体系 (`CMakePresets.json`) 与使用
6. Python 解释器发现与依赖校验流程
7. 第三方依赖管理策略 (FetchContent & 覆盖)
8. 编译配置与关键宏策略
9. 运行时依赖收集与复制 (DLL / PDB)
10. 常用构建与测试命令（含预设示例）
11. 常见问题 (FAQ) 与排查建议
12. 自定义与扩展：添加新预设 / 新模块
13. 最小/快速构建范式

---
## 1. 环境 / 前置依赖
| 组件 | 最低/建议版本 | 说明 |
|------|---------------|------|
| CMake | 3.24+ (文件中声明 4.0 以预留未来特性) | 过低版本可能不支持 Presets v8 功能 |
| 编译器 | MSVC / Clang / GCC (C++20 设定) | 预设中开启 `CMAKE_CXX_STANDARD=20` |
| Python | >= `CORONA_PYTHON_MIN_VERSION` (默认 3.13) | 用于 requirements 依赖校验脚本执行 |
| Git | 最新稳定 | FetchContent 拉取第三方源码 |

补充：
1. Windows 下推荐安装 Ninja 以获得一致/快速的多配置生成体验 (`Ninja Multi-Config`).
2. 若系统 Python 不满足版本需求，可放置嵌入式发行版到 `Env/Python-<ver>` 并开启回退。
3. 可通过 `pip install -r Misc/requirements.txt` 手动预安装以减少配置阶段外网访问。

---
## 2. 仓库目录结构速览
```
CoronaEngine/
  CMakeLists.txt           # 根入口：包含分模块、配置摘要输出
  CMakePresets.json        # 统一预设：生成 / 构建 配置
  Misc/
    cmake/                 # Corona* 模块化脚本
    check_pip_modules.py   # Python 依赖解析 + 自动安装
    requirements.txt       # Python 依赖声明
  Src/                     # 引擎核心 (静态库)
  Examples/                # 示例程序与资源
  Env/                     # 预置或手工放置的外部环境 (TBB / Python / spdlog ...)
  Docs/                    # 文档集合
```

---
## 3. 构建模块分层 (Corona*.cmake) 概览
| 文件 | 角色 | 关键点 |
|------|------|--------|
| `CoronaOptions.cmake` | 构建选项集中定义 | 控制示例构建/依赖检查/自动安装等 |
| `CoronaPython.cmake` | 解释器发现 + requirements 处理 | 系统优先 + 嵌入式回退 + 自定义目标 `check_python_deps` |
| `CoronaRuntimeDeps.cmake` | 收集与复制运行时 DLL/PDB | 属性 `INTERFACE_CORONA_RUNTIME_DEPS` / 分离收集与复制 |
| `CoronaCompileConfig.cmake` | 全局编译宏与运行库策略 | ENTT/FMT/Python 路径宏 / MSVC 运行库统一 |
| `CoronaThirdParty.cmake` | FetchContent 第三方声明 | 可锁定版本或本地 override |
| `Src/CMakeLists.txt` | 核心库构建 | TBB Debug/Release 通过生成器表达式切换 |
| `Examples/CMakeLists.txt` | 示例与资源处理 | 运行时依赖复制 + 资源目录同步 |

---
## 4. CMake 选项与核心变量
| 名称 | 默认 | 说明 |
|------|------|------|
| `BUILD_SHARED_LIBS` | OFF | 切换为 SHARED 时需处理导出符号与安装规则 |
| `CORONA_BUILD_EXAMPLES` | ON | 是否构建示例可执行 |
| `CORONA_CHECK_PY_DEPS` | ON | 是否在配置阶段执行 Python 依赖检查 |
| `CORONA_AUTO_INSTALL_PY_DEPS` | ON | 缺包时自动安装（需网络）失败则报错 |
| `CORONA_PYTHON_MIN_VERSION` | 3.13 | 解释器最低主次版本 |
| `CORONA_PYTHON_USE_EMBEDDED_FALLBACK` | ON | 系统找不到满足版本时尝试内置目录 |
| `CORONA_EMBEDDED_PY_DIR` | Env/Python-3.13.7 | 嵌入式 Python 根目录 |

修改路径：
1. 命令行：`cmake -DCORONA_BUILD_EXAMPLES=OFF ..`
2. 预设扩展：在 `CMakePresets.json` 的 `configurePresets[].cacheVariables` 添加或覆盖。
3. 手工 GUI：CMake GUI 勾选或编辑。

---
## 5. 预设体系 (`CMakePresets.json`) 与使用

### 5.1 预设总览
| 预设名称 | 类型 | 用途 | 继承链 |
|----------|------|------|--------|
| `base` | hidden configure | 基础语言/标准/导出编译数据库 | (根) |
| `win-base` | hidden configure | Windows 专用 (MSVC 运行库策略) | `base` |
| `ninja-mc` | configure | Ninja 多配置生成 | `win-base` |
| `vs2022` | configure | VS 2022 解决方案 | `win-base` |
| `ninja-debug` | build | 构建 Debug (Ninja) | 配置引用 `ninja-mc` |
| `ninja-rel_with_debinfo` | build | 构建 RelWithDebInfo (Ninja) | `ninja-mc` |
| `ninja-release` | build | 构建 Release (Ninja) | `ninja-mc` |
| `ninja-min_size_rel` | build | 构建 MinSizeRel (Ninja) | `ninja-mc` |
| `vs-debug` | build | VS Debug | `vs2022` |
| `vs-rel_with_debinfo` | build | VS RelWithDebInfo | `vs2022` |
| `vs-release` | build | VS Release | `vs2022` |
| `vs-min_size_rel` | build | VS MinSizeRel | `vs2022` |

### 5.2 继承结构示意
```
 base
  └─ win-base (仅在 hostSystemName == Windows 生效)
      ├─ ninja-mc
      │    ├─ build: ninja-debug
      │    ├─ build: ninja-rel_with_debinfo
      │    ├─ build: ninja-release
      │    └─ build: ninja-min_size_rel
      └─ vs2022
           ├─ build: vs-debug
           ├─ build: vs-rel_with_debinfo
           ├─ build: vs-release
           └─ build: vs-min_size_rel
```

### 5.3 使用示例
```powershell
# 配置 (Ninja 多配置)
cmake --preset ninja-mc

# 构建 Debug
cmake --build --preset ninja-debug

# 构建 Release
cmake --build --preset ninja-release

# 配置 VS 解决方案
cmake --preset vs2022
cmake --build --preset vs-rel_with_debinfo
```

### 5.4 覆盖与添加自定义变量
可在命令行对已有预设进一步覆盖：
```powershell
cmake --preset ninja-mc -DCORONA_BUILD_EXAMPLES=OFF -DCORONA_AUTO_INSTALL_PY_DEPS=OFF
```

### 5.5 添加自定义预设示例
在 `configurePresets` 中添加：
```jsonc
{
  "name": "ninja-release-noexamples",
  "inherits": "ninja-mc",
  "cacheVariables": {
    "CORONA_BUILD_EXAMPLES": "OFF",
    "CORONA_CHECK_PY_DEPS": "OFF"
  }
}
```
随后添加可选的 build 预设：
```jsonc
{
  "name": "ninja-release-fast",
  "configurePreset": "ninja-mc",
  "configuration": "Release"
}
```

### 5.6 变量覆盖优先级 (高 → 低)
命令行 `-D` > buildPreset 中的 `cacheVariables` > configurePreset 的 `cacheVariables` > CMakeLists / 模块内部默认值。

---
## 6. Python 解释器发现与依赖校验流程
1. 用户显式指定 (`-DPython3_EXECUTABLE=`) → 直接使用。
2. 未指定：调用 `find_package(Python3 COMPONENTS Interpreter Development)`。
3. 版本不足：若 `CORONA_PYTHON_USE_EMBEDDED_FALLBACK=ON` → 尝试 `CORONA_EMBEDDED_PY_DIR` 下嵌入式。
4. 若仍失败：`FATAL_ERROR` 终止配置。

依赖检查 (`CORONA_CHECK_PY_DEPS=ON` 时)：
- 脚本：`Misc/check_pip_modules.py`
- 读取：`Misc/requirements.txt`
- 逻辑：判断缺失 → 如 `CORONA_AUTO_INSTALL_PY_DEPS=ON` 则自动安装 → 失败终止。
- 可手动执行：`cmake --build . --target check_python_deps`。

快速跳过：`-DCORONA_CHECK_PY_DEPS=OFF`。

---
## 7. 第三方依赖管理策略
当前使用：`assimp`, `stb`, `entt`, `glfw`(示例开启时), `CabbageHardware`。

特性：
- FetchContent 直接源码引入，便于调试与本地修改。
- 建议后续将 `GIT_TAG` 指定为确切 commit，改善可重复性。

本地覆盖（减少下载或定制修改）：
```powershell
set FETCHCONTENT_SOURCE_DIR_assimp=C:/local/assimp
cmake --preset ninja-mc
```

或命令行：`-DFETCHCONTENT_SOURCE_DIR_assimp=C:/local/assimp`。

离线场景：把所需仓库预先 clone 到本地并指向覆盖变量即可。

---
## 8. 编译配置与关键宏策略
集中在 `CoronaCompileConfig.cmake`：
| 宏 | 用途 | 备注 |
|----|------|------|
| `_CRT_SECURE_NO_WARNINGS` | 抑制 MSVC 不安全函数警告 | 保持日志干净 |
| `NOMINMAX` | 关闭 Win 宏 min/max | 防止与 std 冲突 |
| `ENTT_ID_TYPE=uint64_t` | 更大 ECS ID 空间 | ABI 稳定性考虑 |
| `ENTT_USE_ATOMIC` | 启用原子路径 | 多线程安全 |
| `FMT_HEADER_ONLY=1` | header-only 模式 | 免编译独立库 |
| `CORONA_PYTHON_EXE` | 记录解释器路径 | 方便调试/运行期引用 |
| `CORONA_ENGINE_DEBUG/RELEASE` | 编译配置标识 | Feature gating |

MSVC 运行库策略：
- 预设中 `win-base` 选择 `MultiThreadedDLL` (对应 /MD)；如果你希望静态 (/MT) 可在自定义预设覆写：
```jsonc
"cacheVariables": { "CMAKE_MSVC_RUNTIME_LIBRARY": "MultiThreaded$<$<CONFIG:Debug>:Debug>" }
```

---
## 9. 运行时依赖收集与复制
文件：`CoronaRuntimeDeps.cmake`

| 函数 | 说明 | 时机 |
|------|------|------|
| `corona_configure_runtime_deps(target)` | 收集 TBB + Python DLL / PDB 设置到属性 | 定义核心库后调用一次 |
| `corona_install_runtime_deps(exe)` | 构建后复制上一步收集的文件 | 每个需要独立运行的可执行 |

好处：防止忘记拷贝导致运行期缺少 DLL；可扩展添加更多依赖（如 GPU shader compiler 动态库等）。

---
## 10. 常用构建与测试命令（含预设）
### 10.1 使用预设 (推荐)
```powershell
# 配置 + 生成 (Ninja 多配置)
cmake --preset ninja-mc

# 构建 Debug
cmake --build --preset ninja-debug

# 构建 Release
cmake --build --preset ninja-release

# VS 方案
cmake --preset vs2022
cmake --build --preset vs-rel_with_debinfo
```

### 10.2 传统命令行（无预设）
```powershell
mkdir build; cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build . --config RelWithDebInfo
```

### 10.3 覆盖选项
```powershell
cmake --preset ninja-mc -DCORONA_BUILD_EXAMPLES=OFF -DCORONA_CHECK_PY_DEPS=OFF
```

### 10.4 手动触发依赖检查
```powershell
cmake --build . --target check_python_deps
```

---
## 11. 常见问题 (FAQ)
| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| Python 未找到或版本不符 | 系统未安装 / PATH 覆盖 / 版本过低 | 指定 `-DPython3_EXECUTABLE=` 或放置嵌入式并开启回退 |
| 自动安装依赖失败 | 网络受限 / 权限不足 | 手动 `pip install -r Misc/requirements.txt` 后重试，必要时关闭自动安装 |
| 第三方拉取失败 | Git 被阻断 / 无网络 | 预 clone + 使用 `FETCHCONTENT_SOURCE_DIR_*` 覆盖 |
| 运行时缺少 DLL | 未调用复制函数或生成配置不匹配 | 确认执行 `corona_install_runtime_deps`，并检查构建配置一致 |
| VS 调试加载资源失败 | 工作目录错误 | 确认示例工程已设置 `VS_DEBUGGER_WORKING_DIRECTORY` |
| fmt/EnTT 行为与预期不同 | 外部重新定义宏 | 全局搜索重复定义并统一在 `CoronaCompileConfig.cmake` 调整 |
| 构建产物缺少 debug 符号 | 使用 Release 或未生成 PDB | 选择 `RelWithDebInfo` 或 `Debug` 配置 |

---
## 12. 自定义与扩展：添加新预设 / 新模块
场景示例：添加一个“快速 Release（无示例、跳过依赖检查）”预设。
```jsonc
{
  "name": "ninja-release-lite",
  "inherits": "ninja-mc",
  "cacheVariables": {
    "CORONA_BUILD_EXAMPLES": "OFF",
    "CORONA_CHECK_PY_DEPS": "OFF",
    "CMAKE_BUILD_TYPE": "Release"
  }
}
```
可进一步添加 build 预设：
```jsonc
{
  "name": "ninja-release-lite-build",
  "configurePreset": "ninja-release-lite",
  "configuration": "Release"
}
```

新增模块建议：
| 需求 | 建议新文件 | 示例职责 |
|------|------------|----------|
| 安装与导出 | `CoronaInstall.cmake` | `install(TARGETS ...)` + `configure_package_config_file` |
| 测试整合 | `CoronaTests.cmake` | 引入 Catch2/GoogleTest 并注册 ctest |
| 工具脚本 | `CoronaTools.cmake` | 代码生成 / 资源打包命令 |

---
## 13. 最小/快速构建范式
无需示例 + 跳过 Python 依赖检查：
```powershell
cmake --preset ninja-mc -DCORONA_BUILD_EXAMPLES=OFF -DCORONA_CHECK_PY_DEPS=OFF
cmake --build --preset ninja-release
```

或传统：
```powershell
mkdir build_min; cd build_min
cmake -DCORONA_BUILD_EXAMPLES=OFF -DCORONA_CHECK_PY_DEPS=OFF -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```