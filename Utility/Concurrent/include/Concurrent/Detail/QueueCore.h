#pragma once

#include "../Exceptions.h"
#include "Page.h"
#include "PagePool.h"
#include "PauseBackoff.h"
#include "SlotState.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Corona::Concurrent::Detail {

/// \brief 并发队列核心实现，负责页管理、票据分配与同步。
///
/// 对外通过 `ConcurrentQueue` / `ConcurrentBoundedQueue` 等包装类暴露接口，内部使用分页槽位与
/// 自旋+条件变量混合策略，保证多生产者、多消费者环境下的高吞吐与可终止能力。
template <typename T, typename Allocator>
class QueueCore {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using value_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
    using page_type = Page<T>;
    using page_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<page_type>;

    /// \brief 使用外部分配器构造核心组件。
    explicit QueueCore(const Allocator& alloc)
        : value_allocator_(alloc), page_allocator_(alloc), page_pool_(page_allocator_) {}

    QueueCore(const QueueCore&) = delete;
    QueueCore& operator=(const QueueCore&) = delete;

    ~QueueCore() {
        try {
            clear_unsafe();
            page_pool_.release_all();
        } catch (...) {
            // 析构阶段吞并异常。
        }
    }

    /// \brief 将队列标记为终止状态，并唤醒所有等待线程。
    void abort() noexcept {
        aborted_.store(true, std::memory_order_release);
        data_available_cv_.notify_all();
    }

    /// \brief 查询是否已经被终止。
    bool is_aborted() const noexcept { return aborted_.load(std::memory_order_acquire); }

    /// \brief 近似判断队列是否为空。
    bool empty() const noexcept { return size_.load(std::memory_order_acquire) == 0; }

    /// \brief 近似返回当前元素数量。
    std::size_t size() const noexcept { return size_.load(std::memory_order_acquire); }

    /// \brief 清空所有页并回收到页面池。
    void clear() {
        std::vector<page_type*> to_recycle;
        {
            std::unique_lock<std::shared_mutex> lock(pages_mutex_);
            to_recycle.reserve(pages_.size());
            for (auto& [index, page] : pages_) {
                clear_page(page);
                page->reset(index);
                to_recycle.push_back(page);
            }
            pages_.clear();
        }
        for (auto* page : to_recycle) {
            page_pool_.recycle(page);
        }
        head_ticket_.store(0, std::memory_order_release);
        tail_ticket_.store(0, std::memory_order_release);
        size_.store(0, std::memory_order_release);
    }

protected:
    /// \brief 生产一个元素，必要时阻塞等待可写槽位。
    template <typename Producer>
    void produce(Producer&& producer) {
        ensure_not_aborted();
        const auto ticket = tail_ticket_.fetch_add(1, std::memory_order_acq_rel);
        auto* slot = acquire_slot(ticket, /*create_page=*/true);
        PauseBackoff backoff;
        slot->wait_until_empty(backoff);
        producer(*slot, value_allocator_);
        finalize_produce(ticket);
    }

    /// \brief 非阻塞生产，当队列被终止时返回 false。
    template <typename Producer>
    bool try_produce(Producer&& producer) {
        if (is_aborted()) {
            return false;
        }
        const auto ticket = tail_ticket_.fetch_add(1, std::memory_order_acq_rel);
        auto* slot = acquire_slot(ticket, /*create_page=*/true);
        PauseBackoff backoff;
        slot->wait_until_empty(backoff);
        producer(*slot, value_allocator_);
        finalize_produce(ticket);
        return true;
    }

    /// \brief 尝试获取一个消费票据，成功时返回 true 并写入 ticket。
    bool try_reserve_head(std::uint64_t& ticket) {
        std::size_t expected = size_.load(std::memory_order_acquire);
        while (expected != 0) {
            if (size_.compare_exchange_weak(expected, expected - 1, std::memory_order_acq_rel, std::memory_order_acquire)) {
                ticket = head_ticket_.fetch_add(1, std::memory_order_acq_rel);
                return true;
            }
        }
        return false;
    }

    /// \brief 非阻塞消费元素，若队列为空返回 false。
    template <typename Consumer>
    bool try_consume(Consumer&& consumer) {
        std::uint64_t ticket = 0;
        if (!try_reserve_head(ticket)) {
            return false;
        }
        consume_ticket(ticket, std::forward<Consumer>(consumer));
        return true;
    }

    /// \brief 阻塞等待直至成功消费或队列被终止。
    template <typename Consumer>
    void consume_blocking(Consumer&& consumer) {
        while (true) {
            if (is_aborted()) {
                throw QueueAborted("concurrent queue aborted");
            }
            if (try_consume(std::forward<Consumer>(consumer))) {
                return;
            }
            std::unique_lock<std::mutex> lock(wait_mutex_);
            data_available_cv_.wait(lock, [&]() { return is_aborted() || size_.load(std::memory_order_acquire) > 0; });
            if (is_aborted() && size_.load(std::memory_order_acquire) == 0) {
                throw QueueAborted("concurrent queue aborted");
            }
        }
    }

    void notify_item_available() noexcept { data_available_cv_.notify_one(); }
    void notify_all_items() noexcept { data_available_cv_.notify_all(); }

    /// \brief 获取指定页；必要时在持有写锁的情况下创建新页。
    page_type* acquire_page(std::size_t page_index, bool create_if_missing) {
        if (auto* page = find_page(page_index)) {
            return page;
        }
        if (!create_if_missing) {
            return nullptr;
        }
        std::unique_lock<std::shared_mutex> lock(pages_mutex_);
        auto iter = pages_.find(page_index);
        if (iter != pages_.end()) {
            return iter->second;
        }
        auto* page = page_pool_.acquire();
        page->reset(page_index);
        pages_.emplace(page_index, page);
        return page;
    }

    /// \brief 在共享锁保护下尝试查找页面。
    page_type* find_page(std::size_t page_index) const {
        std::shared_lock<std::shared_mutex> lock(pages_mutex_);
        auto iter = pages_.find(page_index);
        return iter == pages_.end() ? nullptr : iter->second;
    }

    /// \brief 当页面所有槽位均消费完毕后回收到页面池。
    void retire_page(std::size_t page_index, page_type* page) {
        {
            std::unique_lock<std::shared_mutex> lock(pages_mutex_);
            auto iter = pages_.find(page_index);
            if (iter != pages_.end() && iter->second == page) {
                pages_.erase(iter);
            }
        }
        page_pool_.recycle(page);
    }

    std::size_t slot_index(std::uint64_t ticket) const noexcept {
        return static_cast<std::size_t>(ticket % page_type::kSlotCount);
    }

    std::size_t page_index(std::uint64_t ticket) const noexcept {
        return static_cast<std::size_t>(ticket / page_type::kSlotCount);
    }

    template <typename Consumer>
    void consume_ticket(std::uint64_t ticket, Consumer&& consumer) {
        auto* page = wait_for_page(page_index(ticket));
        auto* slot = page->slot_at(slot_index(ticket));
        PauseBackoff backoff;
        slot->wait_until_full(backoff);
        slot->mark_consuming();
        consumer(slot->value());
        slot->destroy(value_allocator_);
        page->consumed.fetch_add(1, std::memory_order_acq_rel);
        if (page->fully_consumed()) {
            retire_page(page_index(ticket), page);
        }
    }

    /// \brief 生产者写入完成后更新队列长度并唤醒等待线程。
    void finalize_produce(std::uint64_t) {
        size_.fetch_add(1, std::memory_order_release);
        notify_item_available();
    }

    template <typename SlotType = typename page_type::SlotType>
    SlotType* acquire_slot(std::uint64_t ticket, bool create_page) {
        auto* page = acquire_page(page_index(ticket), create_page);
        return page->slot_at(slot_index(ticket));
    }

    page_type* wait_for_page(std::size_t index) {
        PauseBackoff backoff;
        while (true) {
            if (auto* page = find_page(index)) {
                return page;
            }
            if (is_aborted()) {
                throw QueueAborted("concurrent queue aborted");
            }
            backoff.pause();
        }
    }

    /// \brief 在执行写入前再次确认未被终止。
    void ensure_not_aborted() const {
        if (is_aborted()) {
            throw QueueAborted("concurrent queue aborted");
        }
    }

    /// \brief 不加锁清空当前页映射，主要用于析构流程。
    void clear_unsafe() {
        std::vector<page_type*> to_recycle;
        {
            std::unique_lock<std::shared_mutex> lock(pages_mutex_);
            to_recycle.reserve(pages_.size());
            for (auto& [index, page] : pages_) {
                clear_page(page);
                to_recycle.push_back(page);
            }
            pages_.clear();
        }
        for (auto* page : to_recycle) {
            page_pool_.recycle(page);
        }
        size_.store(0, std::memory_order_release);
    }

    /// \brief 逐槽位释放页面内的剩余对象。
    void clear_page(page_type* page) {
        for (auto& slot : page->slots) {
            const auto state = slot.state();
            if (state == SlotState::kFull || state == SlotState::kConsuming) {
                slot.destroy(value_allocator_);
            } else {
                slot.mark_empty();
            }
        }
        page->consumed.store(0, std::memory_order_relaxed);
    }

protected:
    mutable std::shared_mutex pages_mutex_;
    std::unordered_map<std::size_t, page_type*> pages_;
    value_allocator_type value_allocator_;
    page_allocator_type page_allocator_;
    PagePool<T, page_allocator_type> page_pool_;
    std::atomic<std::uint64_t> head_ticket_{0};
    std::atomic<std::uint64_t> tail_ticket_{0};
    std::atomic<std::size_t> size_{0};
    std::atomic<bool> aborted_{false};
    std::condition_variable data_available_cv_;
    mutable std::mutex wait_mutex_;
};

} // namespace Corona::Concurrent::Detail
