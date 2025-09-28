#pragma once

#include <stdexcept>

namespace Corona::Concurrent {

/// \brief 队列在 `abort()` 后用于通知等待线程的异常类型。
///
/// 当阻塞的 push / pop 检测到外部通过 `abort()` 终止队列时抛出，调用方应捕获并终止当前逻辑或重试。
class QueueAborted final : public std::runtime_error {
public:
    explicit QueueAborted(const char* message)
        : std::runtime_error(message) {}
};

} // namespace Corona::Concurrent
