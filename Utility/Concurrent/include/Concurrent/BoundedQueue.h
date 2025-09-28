#pragma once

#include "Detail/QueueCore.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

namespace Corona::Concurrent {

/// \brief 有界多生产者多消费者队列。
///
/// - 入队操作在队列满时阻塞，直到有消费者释放空间或队列被终止。
/// - 出队操作在队列空且未终止时阻塞，语义与 Condition Variable 队列一致。
/// - 可动态调整容量，内部自动唤醒等待线程。
template <typename T, typename Allocator = std::allocator<T>>
class ConcurrentBoundedQueue : private Detail::QueueCore<T, Allocator> {
    using Base = Detail::QueueCore<T, Allocator>;

public:
    using value_type = T;
    using allocator_type = Allocator;

    /// \brief 构造指定容量的有界队列。
    /// \param capacity 最大可存放元素数量。
    /// \param alloc 元素分配器，用于控制内存来源。
    explicit ConcurrentBoundedQueue(std::size_t capacity, const Allocator& alloc = Allocator{})
        : Base(alloc), capacity_(capacity) {}

    ConcurrentBoundedQueue(const ConcurrentBoundedQueue&) = delete;
    ConcurrentBoundedQueue& operator=(const ConcurrentBoundedQueue&) = delete;

    ConcurrentBoundedQueue(ConcurrentBoundedQueue&&) = delete;
    ConcurrentBoundedQueue& operator=(ConcurrentBoundedQueue&&) = delete;

    ~ConcurrentBoundedQueue() = default;

    /// \brief 动态调整容量，唤醒等待生产者线程。
    void set_capacity(std::size_t capacity) {
        {
            std::lock_guard<std::mutex> guard(capacity_mutex_);
            capacity_ = capacity;
        }
        space_available_cv_.notify_all();
    }

    /// \brief 查询当前容量。
    std::size_t capacity() const noexcept {
        std::lock_guard<std::mutex> guard(capacity_mutex_);
        return capacity_;
    }

    /// \brief 阻塞式入队副本，直到成功或队列终止。
    /// @throws QueueAborted 队列被终止。
    void push(const value_type& value) { enqueue_blocking([&](auto& slot, auto& alloc) { slot.construct(alloc, value); }); }

    /// \brief 阻塞式移动入队。
    /// @throws QueueAborted 队列被终止。
    void push(value_type&& value) { enqueue_blocking([&](auto& slot, auto& alloc) { slot.construct(alloc, std::move(value)); }); }

    /// \brief 在槽位内直接构造元素。
    /// @throws QueueAborted 队列被终止。
    template <typename... Args>
    void emplace(Args&&... args) { enqueue_blocking([&](auto& slot, auto& alloc) { slot.emplace(alloc, std::forward<Args>(args)...); }); }

    /// \brief 非阻塞入队副本。
    bool try_push(const value_type& value) { return enqueue_try([&](auto& slot, auto& alloc) { slot.construct(alloc, value); }); }

    /// \brief 非阻塞移动入队。
    bool try_push(value_type&& value) { return enqueue_try([&](auto& slot, auto& alloc) { slot.construct(alloc, std::move(value)); }); }

    /// \brief 非阻塞 emplace。
    template <typename... Args>
    bool try_emplace(Args&&... args) { return enqueue_try([&](auto& slot, auto& alloc) { slot.emplace(alloc, std::forward<Args>(args)...); }); }

    /// \brief 尝试弹出一个元素，若为空返回 false。
    bool try_pop(value_type& out) {
        bool consumed = Base::try_consume([&](value_type& stored) { out = std::move(stored); });
        if (consumed) {
            space_available_cv_.notify_one();
        }
        return consumed;
    }

    /// \brief 阻塞式弹出，直到取得元素或队列终止。
    /// @throws QueueAborted 队列被终止。
    value_type pop() {
        value_type result;
        Base::consume_blocking([&](value_type& stored) { result = std::move(stored); });
        space_available_cv_.notify_one();
        return result;
    }

    /// \brief 阻塞式弹出到外部引用。
    /// @throws QueueAborted 队列被终止。
    void pop(value_type& out) {
        Base::consume_blocking([&](value_type& stored) { out = std::move(stored); });
        space_available_cv_.notify_one();
    }

    /// \brief 终止队列，唤醒所有等待的生产者和消费者。
    void abort() noexcept {
        Base::abort();
        space_available_cv_.notify_all();
    }

    /// \brief 判断是否已经终止。
    bool is_aborted() const noexcept { return Base::is_aborted(); }

    /// \brief 近似检查是否为空，可能与其他线程并发修改。
    bool empty() const noexcept { return Base::empty(); }

    /// \brief 近似获取长度，仅用于监控或调试。
    std::size_t size() const noexcept { return Base::size(); }

    /// \brief 清空队列并唤醒所有等待线程。
    void clear() {
        Base::clear();
        space_available_cv_.notify_all();
    }

private:
    /// \brief 核心阻塞写入逻辑，满足条件后生产元素。
    template <typename Producer>
    void enqueue_blocking(Producer&& producer) {
        while (true) {
            if (Base::is_aborted()) {
                throw QueueAborted("concurrent bounded queue aborted");
            }
            std::unique_lock<std::mutex> lock(capacity_mutex_);
            space_available_cv_.wait(lock, [&]() {
                return Base::is_aborted() || Base::size() < capacity_;
            });
            if (Base::is_aborted()) {
                throw QueueAborted("concurrent bounded queue aborted");
            }
            Base::produce(std::forward<Producer>(producer));
            return;
        }
    }

    /// \brief 非阻塞写入，当容量不足或队列终止时返回 false。
    template <typename Producer>
    bool enqueue_try(Producer&& producer) {
        std::lock_guard<std::mutex> guard(capacity_mutex_);
        if (Base::size() >= capacity_ || Base::is_aborted()) {
            return false;
        }
        Base::produce(std::forward<Producer>(producer));
        return true;
    }

    mutable std::mutex capacity_mutex_;
    std::condition_variable space_available_cv_;
    std::size_t capacity_;
};

} // namespace Corona::Concurrent
