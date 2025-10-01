#pragma once

#include "../core/atomic.h"
#include "../util/hazard_pointer.h"
#include <optional>

namespace Corona::Concurrent {

/**
 * 高性能多生产者多消费者（MPMC）无锁队列
 * 
 * 基于 Michael & Scott 算法的改进版本，特性：
 * - Lock-free 设计，支持高并发读写
 * - ABA 安全，使用 Hazard Pointer 进行内存回收
 * - 缓存行对齐，避免伪共享
 * - 支持任意类型的数据存储
 * 
 * @tparam T 队列中存储的数据类型
 */
template<typename T>
class MPMCQueue {
private:
    // 队列节点结构
    struct alignas(Core::CACHE_LINE_SIZE) Node {
        Core::Atomic<T*> data{nullptr};        // 数据指针，nullptr 表示空节点
        Core::Atomic<Node*> next{nullptr};     // 指向下一个节点
        
        Node() = default;
        explicit Node(T* item) : data(item) {}
    };

    // 使用完整的HazardPointer实现
    using NodeHazardPointer = HazardPointer<Node>;

    alignas(Core::CACHE_LINE_SIZE) Core::Atomic<Node*> head_;  // 队列头部
    alignas(Core::CACHE_LINE_SIZE) Core::Atomic<Node*> tail_;  // 队列尾部
    alignas(Core::CACHE_LINE_SIZE) Core::AtomicSize size_{0};  // 队列大小（近似值）

public:
    /**
     * 构造函数，创建空队列
     */
    MPMCQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    /**
     * 析构函数，清理所有节点
     */
    ~MPMCQueue() {
        // 清空所有剩余元素
        while (auto item = dequeue()) {
            // 元素已被移出，自动析构
        }
        
        // 删除哨兵节点
        Node* head = head_.load(std::memory_order_relaxed);
        delete head;
    }

    // 禁用拷贝和移动
    MPMCQueue(const MPMCQueue&) = delete;
    MPMCQueue& operator=(const MPMCQueue&) = delete;
    MPMCQueue(MPMCQueue&&) = delete;
    MPMCQueue& operator=(MPMCQueue&&) = delete;

    /**
     * 将元素添加到队列尾部
     * @param item 要添加的元素
     */
    void enqueue(T item) {
        Node* new_node = new Node(new T(std::move(item)));
        typename NodeHazardPointer::Guard tail_guard(0);
        
        for (;;) {
            Node* tail = tail_.load(std::memory_order_acquire);
            tail_guard.protect(tail);
            
            // 再次检查 tail 是否变化（ABA 保护）
            if (tail != tail_.load(std::memory_order_acquire)) {
                continue;
            }
            
            Node* next = tail->next.load(std::memory_order_acquire);
            
            // 再次检查 tail 是否仍然是尾节点
            if (tail == tail_.load(std::memory_order_acquire)) {
                if (next == nullptr) {
                    // 尝试链接新节点到尾部
                    if (tail->next.compare_exchange_weak(next, new_node,
                                                       std::memory_order_release,
                                                       std::memory_order_relaxed)) {
                        // 链接成功，尝试更新 tail 指针
                        tail_.compare_exchange_weak(tail, new_node,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed);
                        size_.fetch_add(1, std::memory_order_relaxed);
                        return;
                    }
                } else {
                    // 帮助其他线程推进 tail 指针
                    tail_.compare_exchange_weak(tail, next,
                                              std::memory_order_release,
                                              std::memory_order_relaxed);
                }
            }
        }
    }

    /**
     * 从队列头部取出元素
     * @return 如果队列非空返回元素，否则返回 nullopt
     */
    std::optional<T> dequeue() {
        typename NodeHazardPointer::Guard head_guard(0);
        typename NodeHazardPointer::Guard next_guard(1);
        
        for (;;) {
            Node* head = head_.load(std::memory_order_acquire);
            head_guard.protect(head);
            
            // 再次检查 head 是否变化
            if (head != head_.load(std::memory_order_acquire)) {
                continue;
            }
            
            Node* tail = tail_.load(std::memory_order_acquire);
            Node* next = head->next.load(std::memory_order_acquire);
            next_guard.protect(next);
            
            // 再次检查 head 是否仍然是头节点
            if (head == head_.load(std::memory_order_acquire)) {
                if (head == tail) {
                    if (next == nullptr) {
                        // 队列为空
                        return std::nullopt;
                    }
                    
                    // 帮助推进 tail 指针
                    tail_.compare_exchange_weak(tail, next,
                                              std::memory_order_release,
                                              std::memory_order_relaxed);
                } else {
                    if (next == nullptr) {
                        // 不一致状态，重试
                        continue;
                    }
                    
                    // 读取数据
                    T* data = next->data.load(std::memory_order_acquire);
                    if (data == nullptr) {
                        // 数据已被其他线程取走，重试
                        continue;
                    }
                    
                    // 尝试推进头指针
                    if (head_.compare_exchange_weak(head, next,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed)) {
                        // 成功获取数据
                        T result = *data;
                        delete data;  // 释放数据内存
                        NodeHazardPointer::retire(head);  // 延迟释放节点
                        size_.fetch_sub(1, std::memory_order_relaxed);
                        return result;
                    }
                }
            }
        }
    }

    /**
     * 尝试从队列头部取出元素（非阻塞）
     * @param item 输出参数，存储取出的元素
     * @return true 如果成功取出元素，false 如果队列为空
     */
    bool try_dequeue(T& item) {
        auto result = dequeue();
        if (result) {
            item = std::move(*result);
            return true;
        }
        return false;
    }

    /**
     * 检查队列是否为空
     * @return true 如果队列为空
     * @note 这是一个近似检查，在并发环境下结果可能立即过时
     */
    bool empty() const noexcept {
        Node* head = head_.load(std::memory_order_acquire);
        Node* tail = tail_.load(std::memory_order_acquire);
        return head == tail && head->next.load(std::memory_order_acquire) == nullptr;
    }

    /**
     * 获取队列中元素的近似数量
     * @return 元素数量（近似值）
     * @note 在高并发环境下，返回值可能不准确
     */
    size_t size() const noexcept {
        return size_.load(std::memory_order_relaxed);
    }
};



} // namespace Corona::Concurrent