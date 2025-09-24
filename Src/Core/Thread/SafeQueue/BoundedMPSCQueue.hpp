// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace Corona
{
    /**
     * @brief 多生产者-单消费者 定长无锁队列。
     *
     * - 生产者侧使用类似 Vyukov 的序号槽位获取方式；
     * - 单消费者侧线性推进 head，无需 CAS；
     * - 容量为 2 的幂，编译期固定。
     */
    template <typename T, std::size_t CapacityPow2>
    class BoundedMPSCQueue
    {
        static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power of two");

      public:
        BoundedMPSCQueue() noexcept
        {
            for (std::size_t i = 0; i < CapacityPow2; ++i)
            {
                cells_[i].seq.store(i, std::memory_order_relaxed);
            }
            head_.store(0, std::memory_order_relaxed);
            tail_.store(0, std::memory_order_relaxed);
        }

        ~BoundedMPSCQueue()
        {
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
            auto head = head_.load(std::memory_order_relaxed);
            cell *c = &cells_[head & mask];
            std::size_t seq = c->seq.load(std::memory_order_acquire);
            intptr_t dif = static_cast<intptr_t>(seq) - static_cast<intptr_t>(head + 1);
            if (dif != 0)
            {
                return false; // empty or busy
            }
            out = std::move(*reinterpret_cast<T*>(&c->storage));
            reinterpret_cast<T*>(&c->storage)->~T();
            head_.store(head + 1, std::memory_order_release);
            c->seq.store(head + CapacityPow2, std::memory_order_release);
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

        alignas(64) std::atomic<std::size_t> head_{0}; // single consumer moves this
        alignas(64) std::atomic<std::size_t> tail_{0}; // multi producers CAS on this
        alignas(64) cell cells_[CapacityPow2];
    };
} // namespace Corona
