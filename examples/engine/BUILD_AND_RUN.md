# 构建和运行 CoronaEngine

## 快速开始

### 1. 配置项目

```powershell
# Windows (使用 Visual Studio)
cmake --preset ninja-msvc

# 或者使用其他预设
cmake --preset <preset-name>
```

### 2. 构建引擎

```powershell
# Debug 模式
cmake --build --preset msvc-debug

# Release 模式
cmake --build --preset msvc-release
```

### 3. 运行主程序

```powershell
# 从构建目录运行
cd build/examples/engine
./corona_engine.exe

# 或者直接运行
./build/examples/engine/Debug/corona_engine.exe
```

## 构建产物

构建完成后，会生成以下文件：

```
build/
├── examples/
│   └── engine/
│       ├── Debug/
│       │   ├── corona_engine.exe        # 主程序
│       │   └── assets/                  # 自动复制的资源文件
│       └── Release/
│           ├── corona_engine.exe
│           └── assets/
└── src/
    ├── CoronaEngine.lib                 # 引擎核心库
    └── systems/
        ├── corona_animation_system.lib  # 动画系统
        ├── corona_audio_system.lib      # 音频系统
        ├── corona_display_system.lib    # 显示系统
        └── corona_rendering_system.lib  # 渲染系统
```

## 运行示例

### 启动引擎

```powershell
PS> ./corona_engine.exe

    ╔══════════════════════════════════════════════════════════════╗
    ║                                                              ║
    ║                      CoronaEngine v0.5.0                     ║
    ║                                                              ║
    ║              A Modern Game Engine Framework                  ║
    ║                                                              ║
    ╚══════════════════════════════════════════════════════════════╝

[Main] Initializing engine...
====================================
CoronaEngine Initializing...
====================================
Registering core systems...
  - DisplaySystem registered (priority 100)
  - RenderingSystem registered (priority 90)
  - AnimationSystem registered (priority 80)
  - AudioSystem registered (priority 70)
All core systems registered
Initialized system: Display
Initialized system: Rendering
Initialized system: Animation
Initialized system: Audio
====================================
CoronaEngine Initialized Successfully
====================================
[Main] Engine initialized successfully

[Main] Starting engine main loop...
[Main] Press Ctrl+C to exit
```

### 退出引擎

按 `Ctrl+C` 退出：

```
^C
[Signal] Interrupt received, requesting engine shutdown...
Engine exit requested
====================================
CoronaEngine Main Loop Exited
====================================

[Main] Shutting down engine...
====================================
CoronaEngine Shutting Down...
====================================
====================================
CoronaEngine Shut Down Complete
====================================
[Main] Engine shutdown complete

╔══════════════════════════════════════════════════════════════╗
║               Thank you for using CoronaEngine!              ║
╚══════════════════════════════════════════════════════════════╝
```

## 故障排除

### 问题：找不到 CoronaFramework

**错误信息：**
```
CMake Error: Could not find CoronaFramework
```

**解决方案：**
确保 CMake 能够找到 CoronaFramework。它会通过 FetchContent 自动下载。

### 问题：找不到 Python

**错误信息：**
```
Could not find Python3
```

**解决方案：**
安装 Python 3.13 或配置 `CORONA_PYTHON_USE_EMBEDDED_FALLBACK=ON`。

### 问题：编译错误

**错误信息：**
```
error C2039: 'xxx': is not a member of 'Corona::Kernel'
```

**解决方案：**
1. 清理构建目录：`rm -r build/`
2. 重新配置：`cmake --preset ninja-msvc`
3. 重新构建：`cmake --build --preset msvc-debug`

## 开发建议

### 调试模式

在 Visual Studio 中：
1. 打开 `CoronaEngine.sln`
2. 设置 `corona_engine` 为启动项目
3. 按 F5 开始调试

### 添加日志

引擎使用 CoronaFramework 的日志系统：

```cpp
auto* logger = engine.logger();
logger->info("Hello World");
logger->warning("Warning message");
logger->error("Error message");
```

### 性能分析

查看系统性能统计：

```cpp
auto* sys_mgr = engine.system_manager();
auto stats = sys_mgr->get_all_stats();

for (const auto& stat : stats) {
    std::cout << "System: " << stat.name << std::endl;
    std::cout << "  FPS: " << stat.actual_fps << std::endl;
    std::cout << "  Frames: " << stat.total_frames << std::endl;
}
```

## 下一步

- 查看 [examples/engine/README.md](README.md) 了解架构详情
- 阅读 [docs/DEVELOPER_GUIDE.zh.md](../../docs/DEVELOPER_GUIDE.zh.md) 学习如何扩展引擎
- 查看其他示例了解更多功能
