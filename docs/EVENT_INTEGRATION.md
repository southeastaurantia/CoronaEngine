# CoronaEngine 事件系统集成说明

## 事件类型

### 1. 系统内部事件（单线程）
使用 **EventBus** 在系统线程内部通信
- `AcousticsSystemDemoEvent`
- `OpticsSystemDemoEvent`
- `MechanicsSystemDemoEvent`
- `GeometrySystemDemoEvent`
- `AnimationSystemDemoEvent`
- `DisplaySystemDemoEvent`
- `EngineDemoEvent`

### 2. 跨线程事件
使用 **EventStream** 在主线程和系统线程之间通信

#### 引擎到系统（主线程 → 系统线程）
- `EngineToAcousticsDemoEvent`
- `EngineToOpticsDemoEvent`
- `EngineToMechanicsDemoEvent`
- `EngineToGeometryDemoEvent`
- `EngineToAnimationDemoEvent`
- `EngineToDisplayDemoEvent`

#### 系统到引擎（系统线程 → 主线程）
- `AcousticsToEngineDemoEvent`
- `OpticsToEngineDemoEvent`
- `MechanicsToEngineDemoEvent`
- `GeometryToEngineDemoEvent`
- `AnimationToEngineDemoEvent`
- `DisplayToEngineDemoEvent`

#### 引擎广播事件（主线程 → 所有系统）
- `FrameBeginEvent` - 帧开始事件
- `FrameEndEvent` - 帧结束事件
- `EngineShutdownEvent` - 引擎关闭事件

## 使用模式

### 系统初始化（initialize）
```cpp
bool XxxSystem::initialize(Kernel::ISystemContext* ctx) {
    // 订阅系统内部事件（EventBus）
    auto* event_bus = ctx->event_bus();
    if (event_bus) {
        event_bus->subscribe<Events::XxxSystemDemoEvent>(...);
    }

    // 订阅跨线程事件（EventStream）
    auto* event_stream = ctx->event_stream();
    if (event_stream) {
        event_stream->subscribe<Events::EngineToXxxDemoEvent>(...);
        event_stream->subscribe<Events::FrameBeginEvent>(...);
    }

    return true;
}
```

### 系统更新（update）
```cpp
void XxxSystem::update() {
    // 发送跨线程事件到引擎（EventStream）
    Events::XxxToEngineDemoEvent event{delta_time()};
    // event_stream->publish(event);

    // 发送系统内部事件（EventBus）
    Events::XxxSystemDemoEvent event{42};
    // event_bus->publish(event);
}
```

### 系统关闭（shutdown）
```cpp
void XxxSystem::shutdown() {
    // 取消所有事件订阅
    // event_bus_subscriptions_.clear();
    // event_stream_subscriptions_.clear();
}
```

### 引擎主循环（Engine::tick）
```cpp
void Engine::tick() {
    // 广播帧开始事件（EventStream）
    Events::FrameBeginEvent frame_begin{frame_number_, last_frame_time_};
    // event_stream->publish(frame_begin);

    // 处理引擎内部事件（EventBus）
    // event_bus->process_events();

    // 广播帧结束事件（EventStream）
    Events::FrameEndEvent frame_end{frame_number_, last_frame_time_};
    // event_stream->publish(frame_end);
}
```

## 关键原则

1. **单线程通信** = EventBus（系统内部）
2. **跨线程通信** = EventStream（主线程 ↔ 系统线程）
3. **订阅管理** = 在 initialize 订阅，在 shutdown 取消
4. **线程安全** = EventStream 自动处理线程安全
