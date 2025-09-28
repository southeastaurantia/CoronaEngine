# Corona Concurrent Queue 模块

该模块提供无锁/低锁的多生产者多消费者队列，实现参考 Intel oneTBB `concurrent_queue`，并结合 Corona 引擎的缓存、页面管理策略进行重写。公开 API 位于 `Utility/Concurrent/include/Concurrent/` 目录，可通过 `find_package` 后直接在引擎或示例中引用。

## 功能概览

- **`ConcurrentQueue<T>`**：无界队列，按需分配页面；适合高吞吐任务分发。
- **`ConcurrentBoundedQueue<T>`**：固定容量队列，push 在满时阻塞、pop 在空时阻塞，可通过 `set_capacity` 动态调整。
- **`QueueAborted`**：当调用 `abort()` 终止队列后，阻塞操作抛出的异常类型。
- **`ConcurrentQueue.h` 兼容层**：提供 `UnboundedQueue`/`BoundedQueue`/`QueueAbort` 别名，便于旧工程平滑迁移。

所有公开方法均提供中文文档注释，可在 IDE 中直接查看行为约束和异常语义。

## 快速上手

```cpp
#include <Concurrent/Queue.h>

Corona::Concurrent::ConcurrentQueue<int> queue;
queue.emplace(42);
int value = queue.pop(); // value == 42

Corona::Concurrent::ConcurrentBoundedQueue<int> bounded(2);
bounded.try_push(1);
bounded.push(2); // 若队列满会阻塞，直到消费者 pop
```

> ⚠️ 提示：`pop()` / `push()` 在队列被终止后会抛出 `QueueAborted`，请在业务层捕获并结束流程。

## 示例程序

- `Examples/concurrent_queue_usage`：展示生产者-消费者模式、`try_push` 背压控制等常见用法。
- `Examples/concurrent_queue_performance`：简单的吞吐测试，用于在不同硬件上评估性能趋势。

编译示例（假设已生成 Ninja 预设）：

```powershell
cmake --build --preset ninja-debug --target Corona_concurrent_queue_usage
cmake --build --preset ninja-debug --target Corona_concurrent_queue_performance
```

运行可执行文件后，可在终端观察调度输出与性能数据。

## 设计要点

- **分页槽位**：借助 `PagePool` 复用内存块，减少频繁的分配/释放。
- **状态机 Slot**：通过 CAS 与 `PauseBackoff` 控制写入/读取状态，降低自旋冲突。
- **终止语义**：`abort()` 只改变状态并唤醒等待线程，不会主动清空队列，方便调用方决定后续处理策略。

如需扩展新的队列变体，可复用 `Detail::QueueCore` 并根据需求定制外层包装类。欢迎在此基础上添加更多同步原语或工具。