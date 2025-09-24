// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <utility>

namespace Corona
{
    /**
     * @brief 单生产者-单消费者 不定长无锁队列（链表，SPSC 安全释放）。
     *
     * - 基于 Michael-Scott 结构（简化），SPSC 下无需 Hazard Pointer；
     * - 生产者仅推进 tail，消费者仅推进 head，使用 release/acquire 保证可见；
     */
    template <typename T>
    class UnboundedSPSCQueue
    {
      public:
        UnboundedSPSCQueue()
        {
            node *n = new node();
            head_.store(n, std::memory_order_relaxed);
            tail_.store(n, std::memory_order_relaxed);
        }

        ~UnboundedSPSCQueue()
        {
            T tmp;
            while (try_pop(tmp)) {}
            node *h = head_.load(std::memory_order_relaxed);
            delete h;
        }

        [[nodiscard]] bool try_push(const T &v) { return emplace(v); }
        [[nodiscard]] bool try_push(T &&v) { return emplace(std::move(v)); }

        template <typename... Args>
        [[nodiscard]] bool emplace(Args &&...args)
        {
            node *n = new node(std::forward<Args>(args)...);
            n->next.store(nullptr, std::memory_order_relaxed);
            node *prev = tail_.load(std::memory_order_relaxed);
            prev->next.store(n, std::memory_order_release);
            tail_.store(n, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool try_pop(T &out)
        {
            node *head = head_.load(std::memory_order_relaxed);
            node *next = head->next.load(std::memory_order_acquire);
            if (next == nullptr) return false;
            out = std::move(next->value);
            head_.store(next, std::memory_order_release);
            delete head;
            return true;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            node *head = head_.load(std::memory_order_acquire);
            return head->next.load(std::memory_order_acquire) == nullptr;
        }

      private:
        struct node
        {
            std::atomic<node *> next{nullptr};
            T value;
            node() = default;
            template <typename... Args>
            explicit node(Args &&...args) : next(nullptr), value(std::forward<Args>(args)...) {}
        };

        alignas(64) std::atomic<node *> head_{nullptr};
        alignas(64) std::atomic<node *> tail_{nullptr};
    };
} // namespace Corona
