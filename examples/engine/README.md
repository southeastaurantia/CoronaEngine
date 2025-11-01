# CoronaEngine - 主程序示例

这是 CoronaEngine 的主要可执行程序，演示了完整的引擎初始化、运行和关闭流程。

## 功能特性

- ✅ 完整的引擎生命周期管理
- ✅ 优雅的信号处理（Ctrl+C）
- ✅ 多线程引擎主循环
- ✅ 自动初始化四个核心系统：
  - DisplaySystem (窗口和输入)
  - RenderingSystem (渲染)
  - AnimationSystem (动画)
  - AudioSystem (音频)

## 构建

```bash
# 配置项目
cmake --preset ninja-msvc

# 构建
cmake --build --preset msvc-debug

# 运行
./build/examples/engine/corona_engine.exe
```

## 使用方式

### 启动引擎

```bash
corona_engine
```

引擎会自动：
1. 初始化 CoronaFramework KernelContext
2. 注册并初始化所有核心系统
3. 启动系统线程
4. 进入主循环

### 退出引擎

按 `Ctrl+C` 可优雅退出引擎。引擎会：
1. 捕获中断信号
2. 请求退出主循环
3. 停止所有系统线程
4. 关闭所有系统
5. 清理资源

## 架构说明

```
main.cpp (主线程)
  │
  ├─ Engine::initialize()
  │   ├─ KernelContext::initialize()
  │   │   ├─ Logger
  │   │   ├─ EventBus
  │   │   ├─ EventBusStream
  │   │   ├─ VirtualFileSystem
  │   │   ├─ PluginManager
  │   │   └─ SystemManager
  │   │
  │   └─ register_systems()
  │       ├─ DisplaySystem (优先级 100)
  │       ├─ RenderingSystem (优先级 90)
  │       ├─ AnimationSystem (优先级 80)
  │       └─ AudioSystem (优先级 70)
  │
  ├─ Engine::run() (独立线程)
  │   ├─ SystemManager::start_all()
  │   │   ├─ DisplaySystem Thread (120 FPS)
  │   │   ├─ RenderingSystem Thread (60 FPS)
  │   │   ├─ AnimationSystem Thread (60 FPS)
  │   │   └─ AudioSystem Thread (60 FPS)
  │   │
  │   └─ Main Loop (60 FPS)
  │       ├─ 计算帧时间
  │       ├─ tick()
  │       └─ 帧号递增
  │
  └─ Engine::shutdown()
      ├─ SystemManager::stop_all()
      ├─ SystemManager::shutdown_all()
      └─ KernelContext::shutdown()
```

## 日志输出示例

```
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

====================================
CoronaEngine Starting Main Loop
====================================
[Main] Press Ctrl+C to exit

... (系统运行) ...

Engine exit requested
====================================
CoronaEngine Main Loop Exited
====================================

====================================
CoronaEngine Shutting Down...
====================================
====================================
CoronaEngine Shut Down Complete
====================================
```

## 扩展引擎

要添加自定义系统：

1. 创建系统类继承 `Corona::Kernel::SystemBase`
2. 实现 `initialize()`, `update()`, `shutdown()`
3. 在 `Engine::register_systems()` 中注册系统
4. 重新编译

示例：
```cpp
class MyCustomSystem : public Corona::Kernel::SystemBase {
public:
    std::string_view get_name() const override { return "MyCustom"; }
    int get_priority() const override { return 50; }
    bool initialize(Kernel::ISystemContext* ctx) override { return true; }
    void update() override { /* 逻辑 */ }
    void shutdown() override { /* 清理 */ }
};
```

## 相关文档

- [引擎架构](../../docs/DEVELOPER_GUIDE.md)
- [系统开发指南](../../docs/DEVELOPER_GUIDE.zh.md)
- [CMake 配置](../../docs/CMAKE_GUIDE.md)
