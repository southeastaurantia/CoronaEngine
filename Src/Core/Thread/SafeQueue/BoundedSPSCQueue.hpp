// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <array>

namespace Corona
{
    /**
     * @brief 单生产者-单消费者 定长无锁队列（环形缓冲，编译期容量）。
     *
     * 设计说明：
     * - SPSC 模型下仅使用两个索引（生产者写 tail、消费者写 head），避免伪共享，使用 acquire/release 保证可见性；
     * - 容量要求为 2 的幂，便于使用掩码取模；
     * - 提供 try_push/try_pop/emplace 接口，失败立即返回不阻塞。
     */
    template <typename T, std::size_t CapacityPow2>
    class BoundedSPSCQueue
    {
        static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be power of two");

      public:
        BoundedSPSCQueue() noexcept
        {
            head_.store(0, std::memory_order_relaxed);
            tail_.store(0, std::memory_order_relaxed);
        }

        ~BoundedSPSCQueue()
        {
            // 非并发场景下清理残留元素
            auto h = head_.load(std::memory_order_relaxed);
            auto t = tail_.load(std::memory_order_relaxed);
            const std::size_t mask = CapacityPow2 - 1;
            while (h != t)
            {
                reinterpret_cast<T*>(&storage_[h & mask])->~T();
                ++h;
            }
        }

        /**
         * @brief 非阻塞入队（拷贝）。
         */
        [[nodiscard]] bool try_push(const T &v) noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            return emplace(v);
        }

        /**
         * @brief 非阻塞入队（移动）。
         */
        [[nodiscard]] bool try_push(T &&v) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            return emplace(std::move(v));
        }

        /**
         * @brief 原地构造入队。
         */
        template <typename... Args>
        [[nodiscard]] bool emplace(Args &&...args)
        {
            const std::size_t mask = CapacityPow2 - 1;
            auto tail = tail_.load(std::memory_order_relaxed);
            auto head = head_cache_;

            // 本地缓存 head，降低对消费者的读取压力
            if (tail - head >= CapacityPow2)
            {
                head = head_.load(std::memory_order_acquire);
                head_cache_ = head;
                if (tail - head >= CapacityPow2) return false; // full
            }

            new (&storage_[tail & mask]) T(std::forward<Args>(args)...);

            // 发布写入
            tail_.store(tail + 1, std::memory_order_release);
            return true;
        }

        /**
         * @brief 非阻塞出队。
         */
        [[nodiscard]] bool try_pop(T &out)
        {
            const std::size_t mask = CapacityPow2 - 1;
            auto head = head_.load(std::memory_order_relaxed);
            auto tail = tail_cache_;

            if (tail <= head)
            {
                tail = tail_.load(std::memory_order_acquire);
                tail_cache_ = tail;
                if (tail <= head) return false; // empty
            }

            auto& slot = storage_[head & mask];
            out = std::move(*reinterpret_cast<T*>(&slot));
            reinterpret_cast<T*>(&slot)->~T();
            head_.store(head + 1, std::memory_order_release);
            return true;
        }

        /** @brief 队列是否为空（近似，SPSC下为准确）。 */
        [[nodiscard]] bool empty() const noexcept
        {
            return size() == 0;
        }

        /** @brief 队列是否已满（SPSC下准确）。 */
        [[nodiscard]] bool full() const noexcept
        {
            return size() >= CapacityPow2;
        }

        /** @brief 近似长度，SPSC 下准确。 */
        std::size_t size() const noexcept
        {
            auto h = head_.load(std::memory_order_acquire);
            auto t = tail_.load(std::memory_order_acquire);
            return t > h ? t - h : 0;
        }

        static constexpr std::size_t capacity() noexcept { return CapacityPow2; }

      private:
        using slot = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

        alignas(64) std::atomic<std::size_t> head_{0};
        alignas(64) std::atomic<std::size_t> tail_{0};
        // 本地缓存，减少跨核读取
        alignas(64) std::size_t head_cache_ = 0;
        alignas(64) std::size_t tail_cache_ = 0;

        alignas(64) std::array<slot, CapacityPow2> storage_{};
    };
} // namespace Corona
