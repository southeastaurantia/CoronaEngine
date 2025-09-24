// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <utility>
#include "HazardPtr.hpp"

namespace Corona
{
    /**
     * @brief 单生产者-多消费者 不定长无锁队列（Michael-Scott + Hazard Pointer 回收）。
     */
    template <typename T>
    class UnboundedSPMCQueue
    {
      public:
        UnboundedSPMCQueue()
        {
            node *n = new node();
            head_.store(n, std::memory_order_relaxed);
            tail_.store(n, std::memory_order_relaxed);
        }

        ~UnboundedSPMCQueue()
        {
            T tmp;
            while (try_pop(tmp)) {}
            node *h = head_.load(std::memory_order_relaxed);
            delete h;
            Hazard::drain();
        }

        [[nodiscard]] bool try_push(const T &v) { return emplace(v); }
        [[nodiscard]] bool try_push(T &&v) { return emplace(std::move(v)); }

        template <typename... Args>
        [[nodiscard]] bool emplace(Args &&...args)
        {
            node *n = new node(std::forward<Args>(args)...);
            n->next.store(nullptr, std::memory_order_relaxed);
            // 单生产者：无需 CAS 推尾
            node *prev = tail_.load(std::memory_order_relaxed);
            prev->next.store(n, std::memory_order_release);
            tail_.store(n, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool try_pop(T &out)
        {
            for (;;)
            {
                typename Hazard::ScopedHazard hz0(0);
                typename Hazard::ScopedHazard hz1(1);
                node *head = Hazard::acquire(0, head_);
                node *next = (head ? Hazard::acquire(1, head->next) : nullptr);
                if (next == nullptr)
                {
                    return false; // empty
                }
                if (head_.compare_exchange_weak(head, next, std::memory_order_release))
                {
                    out = std::move(next->value);
                    Hazard::retire(head);
                    return true;
                }
            }
        }

        [[nodiscard]] bool empty() const noexcept
        {
            node *h = head_.load(std::memory_order_acquire);
            return h->next.load(std::memory_order_acquire) == nullptr;
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

        using Hazard = HazardPtrManager<node, 2, 128>;

        alignas(64) std::atomic<node *> head_{nullptr};
        alignas(64) std::atomic<node *> tail_{nullptr};
    };
} // namespace Corona
