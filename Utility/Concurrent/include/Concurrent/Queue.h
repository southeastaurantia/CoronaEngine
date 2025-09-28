#pragma once

#include "Detail/QueueCore.h"

#include <memory>
#include <utility>

namespace Corona::Concurrent {

/// \brief 并发无界队列实现。
///
/// - 支持多生产者、多消费者同时访问，内部使用分段页与票据编号保证无锁推进。
/// - 接口语义参考 oneTBB `concurrent_queue`，push / pop 在成功前不会丢失元素。
/// - 默认情况下按需分配内存，无容量上限；可通过 `Allocator` 自定义分配策略。
template <typename T, typename Allocator = std::allocator<T>>
class ConcurrentQueue : private Detail::QueueCore<T, Allocator> {
    using Base = Detail::QueueCore<T, Allocator>;

public:
    using value_type = T;
    using allocator_type = Allocator;

    ConcurrentQueue()
        : Base(Allocator{}) {}

    explicit ConcurrentQueue(const Allocator& alloc)
        : Base(alloc) {}

    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

    ConcurrentQueue(ConcurrentQueue&&) = delete;
    ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;

    ~ConcurrentQueue() = default;

    /// \brief 将一个副本写入队列，必要时会阻塞等待空槽。
    /// \param value 被复制保存的元素。
    void push(const value_type& value) {
        Base::produce([&](auto& slot, auto& alloc) { slot.construct(alloc, value); });
    }

    /// \brief 将右值元素原地移动到队列尾部。
    /// \param value 将被移动的右值对象，调用后其状态未定义。
    void push(value_type&& value) {
        Base::produce([&](auto& slot, auto& alloc) { slot.construct(alloc, std::move(value)); });
    }

    /// \brief 直接在槽位内构造元素，避免额外拷贝。
    /// \param args 转发给 `T` 构造函数的参数。
    template <typename... Args>
    void emplace(Args&&... args) {
        Base::produce([&](auto& slot, auto& alloc) { slot.emplace(alloc, std::forward<Args>(args)...); });
    }

    /// \brief 非阻塞入队，若队列被终止则立即返回 false。
    bool try_push(const value_type& value) {
        return Base::try_produce([&](auto& slot, auto& alloc) { slot.construct(alloc, value); });
    }

    /// \brief 非阻塞移动入队，若队列被终止则立即返回 false。
    bool try_push(value_type&& value) {
        return Base::try_produce([&](auto& slot, auto& alloc) { slot.construct(alloc, std::move(value)); });
    }

    /// \brief 非阻塞 emplace，若队列被终止则立即返回 false。
    template <typename... Args>
    bool try_emplace(Args&&... args) {
        return Base::try_produce([&](auto& slot, auto& alloc) { slot.emplace(alloc, std::forward<Args>(args)...); });
    }

    /// \brief 尝试弹出一个元素，若当前为空则返回 false。
    bool try_pop(value_type& out) {
        bool consumed = Base::try_consume([&](value_type& stored) { out = std::move(stored); });
        return consumed;
    }

    /// \brief 阻塞式弹出，直到取得元素或队列被终止。
    /// \return 移动出的元素副本。
    /// @throws QueueAborted 队列被外部 `abort()`，且无待取元素。
    value_type pop() {
        value_type result;
        Base::consume_blocking([&](value_type& stored) { result = std::move(stored); });
        return result;
    }

    /// \brief 阻塞式弹出到外部引用。
    /// @throws QueueAborted 队列被外部 `abort()`，且无待取元素。
    void pop(value_type& out) {
        Base::consume_blocking([&](value_type& stored) { out = std::move(stored); });
    }

    /// \brief 通知所有等待线程立即终止，尚未消费的元素仍保留在队列中。
    void abort() noexcept { Base::abort(); }

    /// \brief 查询是否已经调用过 `abort()`。
    bool is_aborted() const noexcept { return Base::is_aborted(); }

    /// \brief 判断当前是否无元素，但注意可能与其他线程并发修改。
    bool empty() const noexcept { return Base::empty(); }

    /// \brief 近似返回队列长度，仅供监控。
    std::size_t size() const noexcept { return Base::size(); }

    /// \brief 清空队列并回收页面缓存，释放后等待线程会被唤醒。
    void clear() { Base::clear(); }
};

} // namespace Corona::Concurrent
