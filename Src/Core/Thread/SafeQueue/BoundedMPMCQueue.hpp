// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace Corona
{
    /**
     * @brief 多生产者-多消费者 定长无锁队列（Dmitry Vyukov 算法，编译期容量）。
     *
     * - 容量要求为 2 的幂；
     * - 每个槽位维护一个序号用于判断占用状态，避免伪共享并确保无锁推进；
     * - 入队/出队均为无锁自旋，冲突场景下仍可推进（lock-free）。
     */
    template <typename T, std::size_t CapacityPow2>
    class BoundedMPMCQueue
    {
        static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power of two");

      public:
        BoundedMPMCQueue() noexcept
        {
            for (std::size_t i = 0; i < CapacityPow2; ++i)
            {
                cells_[i].seq.store(i, std::memory_order_relaxed);
            }
            head_.store(0, std::memory_order_relaxed);
            tail_.store(0, std::memory_order_relaxed);
        }

        ~BoundedMPMCQueue()
        {
            // 逐个清理剩余元素（非并发）
            auto h = head_.load(std::memory_order_relaxed);
            auto t = tail_.load(std::memory_order_relaxed);
            const std::size_t mask = CapacityPow2 - 1;
            while (h != t)
            {
                reinterpret_cast<T*>(&cells_[h & mask].storage)->~T();
                ++h;
            }
        }

        [[nodiscard]] bool try_push(const T &v) { return emplace(v); }
        [[nodiscard]] bool try_push(T &&v) { return emplace(std::move(v)); }

        template <typename... Args>
        [[nodiscard]] bool emplace(Args &&...args)
        {
            const std::size_t mask = CapacityPow2 - 1;
            cell *c;
            std::size_t pos = tail_.load(std::memory_order_relaxed);
            for (;;)
            {
                c = &cells_[pos & mask];
                std::size_t seq = c->seq.load(std::memory_order_acquire);
                intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
                if (dif == 0)
                {
                    if (tail_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        break;
                    }
                }
                else if (dif < 0)
                {
                    return false; // full
                }
                else
                {
                    pos = tail_.load(std::memory_order_relaxed);
                }
            }

            new (&c->storage) T(std::forward<Args>(args)...);
            c->seq.store(pos + 1, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool try_pop(T &out)
        {
            const std::size_t mask = CapacityPow2 - 1;
            cell *c;
            std::size_t pos = head_.load(std::memory_order_relaxed);
            for (;;)
            {
                c = &cells_[pos & mask];
                std::size_t seq = c->seq.load(std::memory_order_acquire);
                intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
                if (dif == 0)
                {
                    if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        break;
                    }
                }
                else if (dif < 0)
                {
                    return false; // empty
                }
                else
                {
                    pos = head_.load(std::memory_order_relaxed);
                }
            }

            out = std::move(*reinterpret_cast<T*>(&c->storage));
            reinterpret_cast<T*>(&c->storage)->~T();
            c->seq.store(pos + CapacityPow2, std::memory_order_release);
            return true;
        }

        [[nodiscard]] bool empty() const noexcept { return size() == 0; }
        [[nodiscard]] bool full() const noexcept { return size() >= CapacityPow2; }
        std::size_t size() const noexcept
        {
            auto h = head_.load(std::memory_order_acquire);
            auto t = tail_.load(std::memory_order_acquire);
            return t > h ? t - h : 0;
        }
        static constexpr std::size_t capacity() noexcept { return CapacityPow2; }

      private:
        struct cell
        {
            std::atomic<std::size_t> seq{0};
            typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
        };

        alignas(64) std::atomic<std::size_t> head_{0};
        alignas(64) std::atomic<std::size_t> tail_{0};
        alignas(64) cell cells_[CapacityPow2];
    };
} // namespace Corona
