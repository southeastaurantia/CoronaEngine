#pragma once

#include "Page.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace Corona::Concurrent::Detail {

/// \brief 页面池：负责缓存与复用 Page 对象，降低频繁分配开销。
template <typename T, typename PageAllocator>
class PagePool {
public:
    using PageType = Page<T>;
    using allocator_traits = std::allocator_traits<PageAllocator>;

    explicit PagePool(const PageAllocator& alloc)
        : allocator_(alloc), free_list_(nullptr) {}

    PagePool(const PagePool&) = delete;
    PagePool& operator=(const PagePool&) = delete;

    ~PagePool() = default;

    /// \brief 从缓存中获取页面，如无可用则向分配器申请新页面。
    PageType* acquire() {
        PageType* head = free_list_.load(std::memory_order_acquire);
        while (head) {
            PageType* next = head->next_free;
            if (free_list_.compare_exchange_weak(head, next, std::memory_order_acq_rel, std::memory_order_acquire)) {
                return head;
            }
        }

        auto* page = allocator_traits::allocate(allocator_, 1);
        allocator_traits::construct(allocator_, page);
        {
            std::lock_guard<std::mutex> guard(allocation_mutex_);
            allocations_.push_back(page);
        }
        return page;
    }

    /// \brief 将页面放回自由链表，等待下一次复用。
    void recycle(PageType* page) {
        auto* head = free_list_.load(std::memory_order_acquire);
        do {
            page->next_free = head;
        } while (!free_list_.compare_exchange_weak(head, page, std::memory_order_acq_rel, std::memory_order_acquire));
    }

    /// \brief 释放所有已分配页面，一般在队列销毁时调用。
    void release_all() {
        std::vector<PageType*> snapshot;
        {
            std::lock_guard<std::mutex> guard(allocation_mutex_);
            snapshot.swap(allocations_);
        }
        for (auto* page : snapshot) {
            allocator_traits::destroy(allocator_, page);
            allocator_traits::deallocate(allocator_, page, 1);
        }
        free_list_.store(nullptr, std::memory_order_release);
    }

private:
    PageAllocator allocator_;
    std::atomic<PageType*> free_list_;
    std::mutex allocation_mutex_;
    std::vector<PageType*> allocations_;
};

} // namespace Corona::Concurrent::Detail
