#pragma once

#include <optional>
#include <utility>

#include "../core/atomic.h"
#include "../detail/runtime_init.h"
#include "../util/hazard_pointer.h"


namespace Corona::Concurrent {

/**
 * 基于 Michael & Scott 算法的无锁 MPMC 队列实现。
 *
 * 特性：
 * - 使用 std::optional<T> 内联存储元素，避免额外分配
 * - Hazard Pointer 保护退役节点，规避 ABA 问题
 * - 满足多生产者多消费者场景的内存可见性要求
 */
template <typename T>
class MPMCQueue {
   private:
    struct Node {
        Core::Atomic<Node*> next{nullptr};
        std::optional<T> value;

        Node() = default;
    };

    using NodeHazardPointer = HazardPointer<Node>;

    alignas(Core::CACHE_LINE_SIZE) Core::Atomic<Node*> head_;
    alignas(Core::CACHE_LINE_SIZE) Core::Atomic<Node*> tail_;
    alignas(Core::CACHE_LINE_SIZE) Core::AtomicSize size_{0};

   public:
    MPMCQueue() {
        detail::ensure_runtime_initialized();
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    ~MPMCQueue() {
        while (dequeue()) {
        }

        Node* dummy = head_.load(std::memory_order_relaxed);
        delete dummy;
        NodeHazardPointer::force_cleanup_all();
    }

    MPMCQueue(const MPMCQueue&) = delete;
    MPMCQueue& operator=(const MPMCQueue&) = delete;
    MPMCQueue(MPMCQueue&&) = delete;
    MPMCQueue& operator=(MPMCQueue&&) = delete;

    void enqueue(T item) {
        Node* new_node = new Node();
        new_node->value.emplace(std::move(item));
        new_node->next.store(nullptr, std::memory_order_relaxed);

        typename NodeHazardPointer::Guard tail_guard(0);

        while (true) {
            Node* tail = tail_.load(std::memory_order_acquire);
            tail_guard.protect(tail);
            if (tail != tail_.load(std::memory_order_acquire)) {
                continue;
            }

            Node* next = tail->next.load(std::memory_order_acquire);
            if (tail != tail_.load(std::memory_order_acquire)) {
                continue;
            }

            if (next == nullptr) {
                if (tail->next.compare_exchange_weak(next, new_node,
                                                     std::memory_order_release,
                                                     std::memory_order_relaxed)) {
                    tail_.compare_exchange_strong(tail, new_node,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed);
                    size_.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            } else {
                tail_.compare_exchange_weak(tail, next,
                                            std::memory_order_release,
                                            std::memory_order_relaxed);
            }
        }
    }

    std::optional<T> dequeue() {
        typename NodeHazardPointer::Guard head_guard(0);
        typename NodeHazardPointer::Guard next_guard(1);

        while (true) {
            Node* head = head_.load(std::memory_order_acquire);
            head_guard.protect(head);
            if (head != head_.load(std::memory_order_acquire)) {
                continue;
            }

            Node* tail = tail_.load(std::memory_order_acquire);
            Node* next = head->next.load(std::memory_order_acquire);
            next_guard.protect(next);

            if (head != head_.load(std::memory_order_acquire)) {
                continue;
            }

            if (next == nullptr) {
                return std::nullopt;
            }

            if (head == tail) {
                tail_.compare_exchange_weak(tail, next,
                                            std::memory_order_release,
                                            std::memory_order_relaxed);
                continue;
            }

            std::optional<T> value = std::move(next->value);
            if (!value.has_value()) {
                continue;
            }

            if (head_.compare_exchange_weak(head, next,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
                next->value.reset();
                head_guard.reset();
                next_guard.reset();
                NodeHazardPointer::retire(head, [](Node* node) { delete node; });
                size_.fetch_sub(1, std::memory_order_relaxed);
                return value;
            }
        }
    }

    bool try_dequeue(T& item) {
        auto result = dequeue();
        if (result) {
            item = std::move(*result);
            return true;
        }
        return false;
    }

    bool empty() const noexcept {
        Node* head = head_.load(std::memory_order_acquire);
        Node* tail = tail_.load(std::memory_order_acquire);
        if (head != tail) {
            return false;
        }
        return head->next.load(std::memory_order_acquire) == nullptr;
    }

    size_t size() const noexcept {
        return size_.load(std::memory_order_relaxed);
    }
};

}  // namespace Corona::Concurrent