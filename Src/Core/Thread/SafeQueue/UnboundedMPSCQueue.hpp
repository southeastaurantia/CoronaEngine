// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <utility>

namespace Corona
{
    /**
     * @brief 多生产者-单消费者 不定长无锁队列（Michael-Scott）。
     *
     * - 多生产者通过 CAS 追加到尾部；
     * - 单消费者独占删除旧头节点，安全释放无需 Hazard Pointer。
     */
    template <typename T>
    class UnboundedMPSCQueue
    {
      public:
        UnboundedMPSCQueue()
        {
            node *n = new node();
            head_.store(n, std::memory_order_relaxed);
            tail_.store(n, std::memory_order_relaxed);
        }

        ~UnboundedMPSCQueue()
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

            node *prev = nullptr;
            while (true)
            {
                prev = tail_.load(std::memory_order_acquire);
                node *next = prev->next.load(std::memory_order_acquire);
                if (prev == tail_.load(std::memory_order_acquire))
                {
                    if (next == nullptr)
                    {
                        if (prev->next.compare_exchange_weak(next, n, std::memory_order_release))
                        {
                            tail_.compare_exchange_strong(prev, n, std::memory_order_release);
                            return true;
                        }
                    }
                    else
                    {
                        // 推进尾巴指针
                        tail_.compare_exchange_weak(prev, next, std::memory_order_release);
                    }
                }
            }
        }

        [[nodiscard]] bool try_pop(T &out)
        {
            node *head = nullptr;
            while (true)
            {
                head = head_.load(std::memory_order_acquire);
                node *tail = tail_.load(std::memory_order_acquire);
                node *next = head->next.load(std::memory_order_acquire);
                if (head == head_.load(std::memory_order_acquire))
                {
                    if (head == tail)
                    {
                        if (next == nullptr) return false; // empty
                        // 推进尾巴
                        tail_.compare_exchange_weak(tail, next, std::memory_order_release);
                    }
                    else
                    {
                        if (head_.compare_exchange_weak(head, next, std::memory_order_release))
                        {
                            out = std::move(next->value);
                            delete head;
                            return true;
                        }
                    }
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

        alignas(64) std::atomic<node *> head_{nullptr};
        alignas(64) std::atomic<node *> tail_{nullptr};
    };
} // namespace Corona
