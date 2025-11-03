# CoronaEngine 事件系统使用指南

## 概念区分

### EventBus（事件总线）- 单线程即时通信
- **用途**: 线程内部的同步通信
- **特点**: 发布后立即调用所有订阅者，无队列缓冲
- **适用场景**: 主线程内部组件通信、单个系统内部模块通信

### EventStream（事件流）- 跨线程异步通信  
- **用途**: 跨线程的异步通信
- **特点**: 事件放入队列，订阅者按需拉取，支持背压策略
- **适用场景**: 主线程与系统线程通信、系统与系统线程通信

---

## 使用示例

### 1️⃣ 主线程内部 - 使用 EventBus

```cpp
// ============================================================================
// 文件: examples/event_usage/main_thread_eventbus.cpp
// 场景: 主线程内部 UI 组件之间通信
// ============================================================================

#include <corona/engine.h>
#include <corona/kernel/event/i_event_bus.h>
#include <iostream>

using namespace Corona;
using namespace Corona::Kernel;

// 定义事件类型
struct ButtonClickEvent {
    std::string button_name;
    int x, y;
};

struct GameStateChanged {
    enum class State { Menu, Playing, Paused } state;
};

void example_main_thread_eventbus() {
    Engine engine;
    engine.initialize();
    
    auto* bus = engine.event_bus();
    
    // 订阅按钮点击事件
    auto id1 = bus->subscribe<ButtonClickEvent>([](const ButtonClickEvent& e) {
        std::cout << "[UI] Button '" << e.button_name 
                  << "' clicked at (" << e.x << ", " << e.y << ")" << std::endl;
    });
    
    // 订阅游戏状态变化
    auto id2 = bus->subscribe<GameStateChanged>([](const GameStateChanged& e) {
        std::cout << "[Game] State changed to: " << static_cast<int>(e.state) << std::endl;
    });
    
    // 主线程发布事件（立即触发）
    bus->publish(ButtonClickEvent{"StartButton", 100, 200});
    bus->publish(GameStateChanged{GameStateChanged::State::Playing});
    
    // 取消订阅
    bus->unsubscribe(id1);
    bus->unsubscribe(id2);
}
```

---

### 2️⃣ 系统内部 - 使用 EventBus

```cpp
// ============================================================================
// 文件: src/systems/src/rendering/rendering_system.cpp
// 场景: RenderingSystem 内部模块间通信
// ============================================================================

#include <corona/systems/rendering_system.h>

namespace Corona {

// 系统内部事件
struct MeshLoadedEvent {
    uint32_t mesh_id;
    size_t vertex_count;
};

struct ShaderCompiledEvent {
    uint32_t shader_id;
    bool success;
};

bool RenderingSystem::initialize(Kernel::ISystemContext* context) {
    context_ = context;
    
    // 获取系统自己的 EventBus（通过 context）
    auto* bus = context->event_bus();
    
    // 订阅内部事件
    mesh_loaded_id_ = bus->subscribe<MeshLoadedEvent>([this](const MeshLoadedEvent& e) {
        // 网格加载完成，创建 GPU 资源
        create_gpu_buffer(e.mesh_id);
    });
    
    shader_compiled_id_ = bus->subscribe<ShaderCompiledEvent>([this](const ShaderCompiledEvent& e) {
        if (e.success) {
            activate_shader(e.shader_id);
        }
    });
    
    return true;
}

void RenderingSystem::update(float delta_time) {
    auto* bus = context_->event_bus();
    
    // 系统内部发布事件（同步调用订阅者）
    bus->publish(MeshLoadedEvent{123, 5000});
}

void RenderingSystem::shutdown() {
    auto* bus = context_->event_bus();
    bus->unsubscribe(mesh_loaded_id_);
    bus->unsubscribe(shader_compiled_id_);
}

}  // namespace Corona
```

---

### 3️⃣ 主线程 → 系统线程 - 使用 EventStream

```cpp
// ============================================================================
// 文件: examples/event_usage/main_to_system_stream.cpp
// 场景: 主线程发送指令到 RenderingSystem
// ============================================================================

#include <corona/engine.h>
#include <corona/kernel/event/i_event_stream.h>
#include <thread>

using namespace Corona;
using namespace Corona::Kernel;

// 定义跨线程事件
struct RenderCommand {
    enum class Type { LoadMesh, SetCamera, UpdateLight } type;
    std::string resource_path;
};

void example_main_to_system() {
    Engine engine;
    engine.initialize();
    
    // 1️⃣ 获取 EventStream 管理器
    auto* stream_hub = engine.event_stream();
    
    // 2️⃣ 创建事件流
    auto render_stream = stream_hub->get_stream<RenderCommand>();
    
    // 3️⃣ RenderingSystem 订阅（在其线程中）
    auto sub = render_stream->subscribe(EventStreamOptions{
        .max_queue_size = 128,
        .policy = BackpressurePolicy::DropOldest
    });
    
    // 模拟 RenderingSystem 线程
    std::thread rendering_thread([&sub]() {
        while (true) {
            // 非阻塞拉取
            if (auto cmd = sub.try_pop()) {
                std::cout << "[Rendering] Received command: " 
                          << static_cast<int>(cmd->type) << std::endl;
            }
            
            // 或者阻塞等待（带超时）
            if (auto cmd = sub.wait_for(std::chrono::milliseconds(16))) {
                // 处理命令
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    
    // 4️⃣ 主线程发布事件（放入队列）
    render_stream->publish(RenderCommand{
        .type = RenderCommand::Type::LoadMesh,
        .resource_path = "models/player.obj"
    });
    
    render_stream->publish(RenderCommand{
        .type = RenderCommand::Type::SetCamera,
        .resource_path = ""
    });
    
    rendering_thread.detach();
}
```

---

### 4️⃣ 系统 → 系统线程 - 使用 EventStream

```cpp
// ============================================================================
// 文件: examples/event_usage/system_to_system_stream.cpp
// 场景: AnimationSystem 通知 RenderingSystem 骨骼变换更新
// ============================================================================

#include <corona/systems/animation_system.h>
#include <corona/systems/rendering_system.h>

namespace Corona {

// 定义系统间事件
struct BoneTransformUpdated {
    uint32_t entity_id;
    std::vector<float> bone_matrices;
};

// ============================================================
// AnimationSystem: 发布者
// ============================================================
bool AnimationSystem::initialize(Kernel::ISystemContext* context) {
    context_ = context;
    
    // 获取全局 EventStream
    auto* stream_hub = Kernel::KernelContext::instance().event_stream();
    bone_update_stream_ = stream_hub->get_stream<BoneTransformUpdated>();
    
    return true;
}

void AnimationSystem::update(float delta_time) {
    // 更新骨骼动画...
    std::vector<float> matrices = compute_bone_transforms();
    
    // 发布到 EventStream（跨线程异步）
    bone_update_stream_->publish(BoneTransformUpdated{
        .entity_id = 42,
        .bone_matrices = std::move(matrices)
    });
}

// ============================================================
// RenderingSystem: 订阅者
// ============================================================
bool RenderingSystem::initialize(Kernel::ISystemContext* context) {
    context_ = context;
    
    // 订阅 AnimationSystem 的事件
    auto* stream_hub = Kernel::KernelContext::instance().event_stream();
    auto bone_stream = stream_hub->get_stream<BoneTransformUpdated>();
    
    bone_subscription_ = bone_stream->subscribe(EventStreamOptions{
        .max_queue_size = 64,
        .policy = BackpressurePolicy::Block
    });
    
    return true;
}

void RenderingSystem::update(float delta_time) {
    // 拉取所有待处理的骨骼更新
    while (auto event = bone_subscription_.try_pop()) {
        // 更新 GPU 骨骼缓冲区
        upload_bone_matrices(event->entity_id, event->bone_matrices);
    }
    
    // 渲染帧...
}

void RenderingSystem::shutdown() {
    bone_subscription_.close();
}

}  // namespace Corona
```

---

## 关键要点总结

| 通信场景 | 使用组件 | 获取方式 | 特点 |
|---------|---------|---------|------|
| 主线程内部 | `IEventBus` | `engine.event_bus()` | 同步即时调用 |
| 系统内部 | `IEventBus` | `context->event_bus()` | 单线程，无队列 |
| 主线程 → 系统 | `EventStream<T>` | `engine.event_stream()->get_stream<T>()` | 异步队列 |
| 系统 → 系统 | `EventStream<T>` | `KernelContext::instance().event_stream()` | 跨线程安全 |

## 最佳实践

1. **EventBus 用于单线程内部**：避免跨线程调用，因为回调在发布者线程执行
2. **EventStream 用于跨线程**：发布者和订阅者解耦，订阅者控制消费速度
3. **选择合适的背压策略**：
   - `Block`: 关键数据不能丢失
   - `DropOldest`: 实时性优先（如输入事件）
   - `DropNewest`: 保留历史数据
4. **RAII 管理订阅**：`EventSubscription` 对象析构时自动取消订阅
5. **避免死锁**：不要在 EventBus 回调中发布同类型事件
