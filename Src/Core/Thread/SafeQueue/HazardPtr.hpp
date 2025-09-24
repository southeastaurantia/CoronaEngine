// SPDX-License-Identifier: MIT
#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <vector>
#include <algorithm>

namespace Corona
{
    /**
     * @brief 简易 Hazard Pointer 管理器（仅用于队列节点回收），支持固定最大线程与每线程固定指针槽位。
     *
     * 设计目的：为 MPMC 不定长队列（Michael-Scott）提供最小化内存回收安全保障，避免无保护释放导致的悬垂指针。
     *
     * 特性与限制：
     * - 固定最大线程数与每线程指针槽位数量（模板参数）；
     * - 注册为“按需懒注册”，线程首次使用自动占用一个槽位；
     * - 仅支持统一的节点类型 `TNode`，回收方式为 `delete TNode*`；
     * - 简化实现，采用顺序一致内存序，牺牲少量性能以换取正确性与实现清晰；
     * - 非严格无锁：回收时使用 `std::vector` 进行临时集合构建，可能触发分配；
     * - 满足高并发常见场景需求，若需更极端性能可后续替换为更完整的HP/epoch方案。
     */
    template <typename TNode, std::size_t KSlots = 2, std::size_t MaxThreads = 128>
    class HazardPtrManager
    {
      public:
        /** @brief 每线程记录结构 */
        struct thread_record
        {
            std::atomic<bool> active{false};
            std::array<std::atomic<void *>, KSlots> hazards;
            std::vector<void *> retired; // 待回收列表

            thread_record()
            {
                for (auto &h : hazards) h.store(nullptr, std::memory_order_relaxed);
                retired.reserve(64);
            }
        };

        /**
         * @brief 获取当前线程的记录（懒注册）。
         */
        static std::array<thread_record, MaxThreads> &all_records() noexcept
        {
            static std::array<thread_record, MaxThreads> records{};
            return records;
        }

        static thread_record &thread_rec() noexcept
        {
            thread_local static thread_record *self = nullptr;
            if (self != nullptr) return *self;

            thread_local static std::size_t self_idx = static_cast<std::size_t>(-1);
            if (self_idx != static_cast<std::size_t>(-1))
            {
                self = &all_records()[self_idx];
                return *self;
            }

            auto &records = all_records();
            // 优先使用原子增量快速获取槽位
            std::size_t idx = next_record_idx_.fetch_add(1, std::memory_order_relaxed);
            if (idx < MaxThreads)
            {
                self_idx = idx;
                records[self_idx].active.store(true, std::memory_order_release);
                self = &records[self_idx];
                return *self;
            }

            // 如果超出，回退到扫描查找（可能有线程已退出）
            for (std::size_t i = 0; i < MaxThreads; ++i)
            {
                bool expected = false;
                if (records[i].active.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
                {
                    self_idx = i;
                    self = &records[i];
                    return *self;
                }
            }

            // 若所有槽位都被占用，则退化使用最后一个槽位
            self_idx = MaxThreads - 1;
            self = &records[self_idx];
            return *self;
        }

        /**
         * @brief 保护一个指针，发布到当前线程的第 slot 个 hazard 槽。
         * @param slot 槽位 [0, KSlots)
         * @param p    需要保护的指针
         * @return 同步后读取到的一致指针（便于调用处重读验证）
         */
        static TNode *protect(std::size_t slot, TNode *p) noexcept
        {
            auto &rec = thread_rec();
            rec.hazards[slot].store(p, std::memory_order_release);
            return p;
        }

        /**
         * @brief 清除当前线程第 slot 个 hazard 指针。
         */
        static void clear(std::size_t slot) noexcept
        {
            auto &rec = thread_rec();
            rec.hazards[slot].store(nullptr, std::memory_order_release);
        }

        /**
         * @brief 原子性地获取一个受保护的指针：读取 `target`，写入 hazard，再次确认未变化；若变化则重试。
         * @param slot 当前线程使用的 hazard 槽位索引
         * @param target 需要保护的原子指针（如 std::atomic<TNode*>）
         * @return 一个与 hazard 一致的稳定快照（或 nullptr）
         */
        static TNode *acquire(std::size_t slot, std::atomic<TNode *> &target) noexcept
        {
            for (;;)
            {
                TNode *p = target.load(std::memory_order_acquire);
                protect(slot, p);
                if (p == target.load(std::memory_order_acquire))
                {
                    return p;
                }
                // 指针变化，重试
            }
        }

        /**
         * @brief RAII 风格的 hazard 保护器：构造时设置、析构时清除。
         */
        class ScopedHazard
        {
          public:
            explicit ScopedHazard(std::size_t slot) noexcept : slot_{slot} {}
            ScopedHazard(const ScopedHazard &) = delete;
            ScopedHazard &operator=(const ScopedHazard &) = delete;
            ScopedHazard(ScopedHazard &&rhs) noexcept : slot_{rhs.slot_} { rhs.slot_ = static_cast<std::size_t>(-1); }
            ScopedHazard &operator=(ScopedHazard &&rhs) noexcept
            {
                if (this != &rhs)
                {
                    clear();
                    slot_ = rhs.slot_;
                    rhs.slot_ = static_cast<std::size_t>(-1);
                }
                return *this;
            }
            ~ScopedHazard() { clear(); }

            void set(TNode *p) noexcept
            {
                if (slot_ != static_cast<std::size_t>(-1)) protect(slot_, p);
            }
            void clear() noexcept
            {
                if (slot_ != static_cast<std::size_t>(-1)) HazardPtrManager::clear(slot_);
            }

          private:
            std::size_t slot_;
        };

        /**
         * @brief 延迟回收一个节点（加入待回收列表）。达到阈值后触发扫描并释放未被任何线程保护的节点。
         */
        static void retire(TNode *node) noexcept
        {
            if (node == nullptr) return;
            auto &rec = thread_rec();
            rec.retired.push_back(node);
            if (rec.retired.size() >= reclaim_threshold_.load(std::memory_order_relaxed))
            {
                try_reclaim(rec);
            }
        }

        /**
         * @brief 强制执行一次回收扫描（析构或收尾清理可调用）。
         */
        static void drain() noexcept
        {
            auto &rec = thread_rec();
            try_reclaim(rec, /*force_all=*/true);
        }

        /**
         * @brief 释放当前线程的注册信息：清空 hazard，主动回收后标记为非活动。
         * 注意：仅应在线程生命周期结束时调用。
         */
        static void release_current_thread() noexcept
        {
            auto &rec = thread_rec();
            for (auto &h : rec.hazards) h.store(nullptr, std::memory_order_release);
            try_reclaim(rec, /*force_all=*/true);
            rec.active.store(false, std::memory_order_release);
        }

        /** @brief 调整回收阈值（默认 64）。*/
        static void set_threshold(std::size_t n) noexcept { reclaim_threshold_.store(n, std::memory_order_relaxed); }
        /** @brief 获取当前回收阈值。*/
        static std::size_t threshold() noexcept { return reclaim_threshold_.load(std::memory_order_relaxed); }
        /** @brief 当前线程待回收数量。*/
        static std::size_t retired_size() noexcept { return thread_rec().retired.size(); }
        /** @brief 活跃线程估计数。*/
        static std::size_t active_threads() noexcept
        {
            std::size_t n = 0;
            for (auto &r : all_records())
            {
                if (r.active.load(std::memory_order_acquire)) ++n;
            }
            return n;
        }

      private:
        static std::atomic<std::size_t> next_record_idx_;
        static std::atomic<std::size_t> reclaim_threshold_;

        static void try_reclaim(thread_record &self, bool force_all = false) noexcept
        {
            if (self.retired.empty()) return;

            // 为 hazard 快照使用线程局部存储以避免重复分配
            thread_local std::vector<void *> hazards_snapshot;
            hazards_snapshot.clear();
            hazards_snapshot.reserve(MaxThreads * KSlots);

            for (auto &rec : all_records())
            {
                if (!rec.active.load(std::memory_order_acquire)) continue;
                for (auto const &hp : rec.hazards)
                {
                    void *p = hp.load(std::memory_order_acquire);
                    if (p != nullptr) hazards_snapshot.push_back(p);
                }
            }

            // 将未被保护的节点释放
            auto it = self.retired.begin();
            while (it != self.retired.end())
            {
                void *p = *it;
                bool protected_now = std::find(hazards_snapshot.begin(), hazards_snapshot.end(), p) != hazards_snapshot.end();
                if (!protected_now)
                {
                    delete static_cast<TNode *>(p);
                    it = self.retired.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            if (force_all)
            {
                // 再尝试一次：如果仍有剩余，直接释放（仅用于进程退出等场景，风险自负）
                for (void *p : self.retired)
                {
                    delete static_cast<TNode *>(p);
                }
                self.retired.clear();
            }
        }
    };

    template <typename TNode, std::size_t KSlots, std::size_t MaxThreads>
    std::atomic<std::size_t> HazardPtrManager<TNode, KSlots, MaxThreads>::next_record_idx_{0};

    template <typename TNode, std::size_t KSlots, std::size_t MaxThreads>
    std::atomic<std::size_t> HazardPtrManager<TNode, KSlots, MaxThreads>::reclaim_threshold_{64};
} // namespace Corona
