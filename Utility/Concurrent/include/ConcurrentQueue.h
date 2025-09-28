#pragma once

#include "Concurrent/BoundedQueue.h"
#include "Concurrent/Exceptions.h"
#include "Concurrent/Queue.h"

#include <memory>

namespace Corona::Concurrent {

/// \brief 兼容旧版 include 的快捷别名头文件。
///
/// - `UnboundedQueue` 与 `BoundedQueue` 分别映射到新的 PascalCase 命名空间实现。
/// - `QueueAbort` 与旧实现保持名称，便于增量迁移。
template <typename T, typename Allocator = std::allocator<T>>
using UnboundedQueue = ConcurrentQueue<T, Allocator>;

template <typename T, typename Allocator = std::allocator<T>>
using BoundedQueue = ConcurrentBoundedQueue<T, Allocator>;

using QueueAbort = QueueAborted;

} // namespace Corona::Concurrent
