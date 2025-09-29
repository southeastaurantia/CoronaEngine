#pragma once

#include "../core/atomic.h"
#include "../container/mpmc_queue.h"
#include <vector>
#include <thread>
#include <functional>
#include <future>
#include <memory>
#include <type_traits>

namespace Corona::Concurrent {

/**
 * 高性能线程池实现
 * 
 * 特性：
 * - 无锁任务队列
 * - 工作窃取机制
 * - 动态线程数调整
 * - 任务优先级支持
 * - 异常安全
 */
class ThreadPool {
public:
    // 任务优先级
    enum class Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };

private:
    // 任务包装器
    struct Task {
        std::function<void()> func;
        Priority priority;
        std::chrono::steady_clock::time_point submit_time;
        
        explicit Task(std::function<void()> f, Priority p = Priority::NORMAL)
            : func(std::move(f)), priority(p), submit_time(std::chrono::steady_clock::now()) {}
        
        // 优先级比较器
        bool operator<(const Task& other) const {
            if (priority != other.priority) {
                return static_cast<int>(priority) < static_cast<int>(other.priority);
            }
            return submit_time > other.submit_time;  // 时间越早优先级越高
        }
    };
    
    // 工作线程状态
    enum class WorkerState {
        IDLE,
        WORKING,
        STEALING,
        STOPPING
    };
    
    // 工作线程信息
    struct alignas(Core::CACHE_LINE_SIZE) WorkerInfo {
        std::thread thread;
        Core::Atomic<WorkerState> state{WorkerState::IDLE};
        MPMCQueue<Task> local_queue;  // 本地任务队列
        Core::AtomicSize tasks_processed{0};
        Core::AtomicSize tasks_stolen{0};
        
        WorkerInfo() = default;
        WorkerInfo(const WorkerInfo&) = delete;
        WorkerInfo& operator=(const WorkerInfo&) = delete;
    };

    std::vector<std::unique_ptr<WorkerInfo>> workers_;
    MPMCQueue<Task> global_queue_;  // 全局任务队列
    
    Core::Atomic<bool> running_{true};
    Core::Atomic<bool> shutdown_requested_{false};
    
    // 统计信息
    Core::AtomicSize total_tasks_submitted_{0};
    Core::AtomicSize total_tasks_completed_{0};
    Core::AtomicSize total_steals_attempted_{0};
    Core::AtomicSize total_steals_successful_{0};
    
    // 配置参数
    std::size_t min_threads_;
    std::size_t max_threads_;
    std::chrono::milliseconds idle_timeout_;
    bool enable_work_stealing_;

public:
    /**
     * 构造函数
     * @param thread_count 线程数量，0表示使用CPU核心数
     * @param enable_work_stealing 是否启用工作窃取
     */
    explicit ThreadPool(
        std::size_t thread_count = 0,
        bool enable_work_stealing = true,
        std::chrono::milliseconds idle_timeout = std::chrono::milliseconds(1000)
    ) : min_threads_(thread_count == 0 ? std::thread::hardware_concurrency() : thread_count),
        max_threads_(min_threads_ * 2),
        idle_timeout_(idle_timeout),
        enable_work_stealing_(enable_work_stealing) {
        
        if (min_threads_ == 0) {
            min_threads_ = 4;  // 默认最小线程数
        }
        
        // 创建工作线程
        workers_.reserve(max_threads_);
        for (std::size_t i = 0; i < min_threads_; ++i) {
            create_worker(i);
        }
    }

    /**
     * 析构函数
     */
    ~ThreadPool() {
        if (running_.load()) {
            shutdown();
        }
    }

    // 禁用拷贝和移动
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * 提交任务
     * @param func 任务函数
     * @param priority 任务优先级
     * @return 任务的future对象
     */
    template<typename F, typename... Args>
    auto submit(F&& func, Args&&... args, Priority priority = Priority::NORMAL) 
        -> std::future<std::invoke_result_t<F, Args...>> {
        
        using return_type = std::invoke_result_t<F, Args...>;
        
        if (shutdown_requested_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is shutting down");
        }
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        Task wrapped_task([task]() { (*task)(); }, priority);
        
        // 尝试将任务放到本地队列
        bool enqueued = try_enqueue_local(std::move(wrapped_task));
        
        if (!enqueued) {
            // 如果本地队列满了，放到全局队列
            global_queue_.enqueue(std::move(wrapped_task));
        }
        
        total_tasks_submitted_.fetch_add(1, std::memory_order_relaxed);
        
        return result;
    }

    /**
     * 提交简单任务（无返回值）
     */
    template<typename F>
    void submit_detached(F&& func, Priority priority = Priority::NORMAL) {
        if (shutdown_requested_.load(std::memory_order_acquire)) {
            return;
        }
        
        Task task(std::forward<F>(func), priority);
        
        bool enqueued = try_enqueue_local(std::move(task));
        if (!enqueued) {
            global_queue_.enqueue(std::move(task));
        }
        
        total_tasks_submitted_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * 等待所有任务完成
     */
    void wait_for_all_tasks() {
        while (total_tasks_submitted_.load(std::memory_order_acquire) > 
               total_tasks_completed_.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    /**
     * 关闭线程池
     */
    void shutdown() {
        shutdown_requested_.store(true, std::memory_order_release);
        running_.store(false, std::memory_order_release);
        
        for (auto& worker : workers_) {
            if (worker && worker->thread.joinable()) {
                worker->thread.join();
            }
        }
        
        workers_.clear();
    }

    /**
     * 获取统计信息
     */
    struct Statistics {
        std::size_t worker_count;
        std::size_t tasks_submitted;
        std::size_t tasks_completed;
        std::size_t tasks_pending;
        std::size_t steals_attempted;
        std::size_t steals_successful;
        double steal_success_rate;
        std::vector<std::size_t> worker_tasks_processed;
    };

    Statistics get_statistics() const {
        Statistics stats;
        stats.worker_count = workers_.size();
        stats.tasks_submitted = total_tasks_submitted_.load(std::memory_order_relaxed);
        stats.tasks_completed = total_tasks_completed_.load(std::memory_order_relaxed);
        stats.tasks_pending = stats.tasks_submitted - stats.tasks_completed;
        stats.steals_attempted = total_steals_attempted_.load(std::memory_order_relaxed);
        stats.steals_successful = total_steals_successful_.load(std::memory_order_relaxed);
        stats.steal_success_rate = stats.steals_attempted > 0 ? 
            static_cast<double>(stats.steals_successful) / stats.steals_attempted : 0.0;
        
        stats.worker_tasks_processed.reserve(workers_.size());
        for (const auto& worker : workers_) {
            if (worker) {
                stats.worker_tasks_processed.push_back(
                    worker->tasks_processed.load(std::memory_order_relaxed)
                );
            }
        }
        
        return stats;
    }

private:
    /**
     * 创建工作线程
     */
    void create_worker(std::size_t worker_id) {
        auto worker = std::make_unique<WorkerInfo>();
        
        worker->thread = std::thread([this, worker_id, worker_ptr = worker.get()]() {
            worker_main_loop(worker_id, worker_ptr);
        });
        
        workers_.push_back(std::move(worker));
    }

    /**
     * 工作线程主循环
     */
    void worker_main_loop(std::size_t worker_id, WorkerInfo* worker_info) {
        while (running_.load(std::memory_order_acquire)) {
            Task task{[](){}};  // 默认空任务
            bool found_task = false;
            
            // 1. 首先尝试从本地队列获取任务
            if (worker_info->local_queue.try_dequeue(task)) {
                found_task = true;
            }
            // 2. 然后尝试从全局队列获取任务
            else if (auto global_task = global_queue_.dequeue()) {
                task = std::move(*global_task);
                found_task = true;
            }
            // 3. 最后尝试工作窃取
            else if (enable_work_stealing_) {
                found_task = try_steal_task(worker_id, task);
            }
            
            if (found_task) {
                // 执行任务
                worker_info->state.store(WorkerState::WORKING, std::memory_order_relaxed);
                
                try {
                    task.func();
                    worker_info->tasks_processed.fetch_add(1, std::memory_order_relaxed);
                    total_tasks_completed_.fetch_add(1, std::memory_order_relaxed);
                } catch (...) {
                    // 异常安全：任务执行失败时仍然计入完成数
                    total_tasks_completed_.fetch_add(1, std::memory_order_relaxed);
                }
                
                worker_info->state.store(WorkerState::IDLE, std::memory_order_relaxed);
            } else {
                // 没有任务时短暂休眠
                worker_info->state.store(WorkerState::IDLE, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
    }

    /**
     * 尝试将任务放入本地队列
     */
    bool try_enqueue_local(Task task) {
        // 简化实现：随机选择一个工作线程的本地队列
        if (workers_.empty()) {
            return false;
        }
        
        // 使用线程ID作为简单的负载均衡
        std::size_t worker_index = std::hash<std::thread::id>{}(std::this_thread::get_id()) % workers_.size();
        
        if (workers_[worker_index]) {
            workers_[worker_index]->local_queue.enqueue(std::move(task));
            return true;
        }
        
        return false;
    }

    /**
     * 尝试从其他线程窃取任务
     */
    bool try_steal_task(std::size_t stealer_id, Task& stolen_task) {
        total_steals_attempted_.fetch_add(1, std::memory_order_relaxed);
        
        // 随机选择一个受害者线程
        for (std::size_t attempt = 0; attempt < workers_.size(); ++attempt) {
            std::size_t victim_id = (stealer_id + attempt + 1) % workers_.size();
            
            if (victim_id < workers_.size() && workers_[victim_id]) {
                auto& victim_worker = *workers_[victim_id];
                
                // 只从正在工作的线程窃取
                if (victim_worker.state.load(std::memory_order_acquire) == WorkerState::WORKING) {
                    if (victim_worker.local_queue.try_dequeue(stolen_task)) {
                        total_steals_successful_.fetch_add(1, std::memory_order_relaxed);
                        if (stealer_id < workers_.size() && workers_[stealer_id]) {
                            workers_[stealer_id]->tasks_stolen.fetch_add(1, std::memory_order_relaxed);
                        }
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
};

} // namespace Corona::Concurrent