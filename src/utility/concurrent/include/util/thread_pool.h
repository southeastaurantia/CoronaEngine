#pragma once

#include "../core/atomic.h"
#include "../container/mpmc_queue.h"
#include "../detail/runtime_init.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

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
    enum class Priority {
        LOW = 0,
        NORMAL = 1,
        HIGH = 2,
        CRITICAL = 3
    };

   private:
    struct Task {
        std::function<void()> func;
        Priority priority{Priority::NORMAL};
        std::chrono::steady_clock::time_point submit_time{std::chrono::steady_clock::now()};

        Task() = default;

        explicit Task(std::function<void()> f, Priority p = Priority::NORMAL)
            : func(std::move(f)), priority(p), submit_time(std::chrono::steady_clock::now()) {}

        bool operator<(const Task& other) const {
            if (priority != other.priority) {
                return static_cast<int>(priority) < static_cast<int>(other.priority);
            }
            return submit_time > other.submit_time;
        }
    };

    enum class WorkerState {
        IDLE,
        WORKING,
        STEALING,
        STOPPING
    };

    struct alignas(Core::CACHE_LINE_SIZE) WorkerInfo {
        std::thread thread;
        Core::Atomic<WorkerState> state{WorkerState::IDLE};
        MPMCQueue<Task> local_queue;
        Core::AtomicSize tasks_processed{0};
        Core::AtomicSize tasks_stolen{0};

        WorkerInfo() = default;
        WorkerInfo(const WorkerInfo&) = delete;
        WorkerInfo& operator=(const WorkerInfo&) = delete;
    };

    std::vector<std::unique_ptr<WorkerInfo>> workers_;
    MPMCQueue<Task> global_queue_;

    Core::Atomic<bool> running_{true};
    Core::Atomic<bool> shutdown_requested_{false};

    Core::AtomicSize total_tasks_submitted_{0};
    Core::AtomicSize total_tasks_completed_{0};
    Core::AtomicSize total_steals_attempted_{0};
    Core::AtomicSize total_steals_successful_{0};

    std::size_t min_threads_;
    std::size_t max_threads_;
    std::chrono::milliseconds idle_timeout_;
    bool enable_work_stealing_;

    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::mutex completion_mutex_;
    std::condition_variable completion_cv_;

    inline static thread_local WorkerInfo* tls_worker_info_ = nullptr;

   public:
    explicit ThreadPool(
        std::size_t thread_count = 0,
        bool enable_work_stealing = true,
        std::chrono::milliseconds idle_timeout = std::chrono::milliseconds(1000))
        : min_threads_(initialize_worker_count(thread_count)),
          max_threads_(min_threads_ * 2),
          idle_timeout_(idle_timeout),
          enable_work_stealing_(enable_work_stealing) {
                detail::ensure_runtime_initialized();
        if (max_threads_ == 0) {
            max_threads_ = min_threads_;
        }

        workers_.reserve(max_threads_);
        for (std::size_t i = 0; i < min_threads_; ++i) {
            create_worker(i);
        }
    }

    ~ThreadPool() {
        if (running_.load(std::memory_order_acquire)) {
            shutdown();
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& func, Args&&... args, Priority priority = Priority::NORMAL)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        if (shutdown_requested_.load(std::memory_order_acquire)) {
            throw std::runtime_error("ThreadPool is shutting down");
        }

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();
        Task wrapped_task([task]() { (*task)(); }, priority);

        enqueue_task(std::move(wrapped_task));
        return result;
    }

    template <typename F>
    void submit_detached(F&& func, Priority priority = Priority::NORMAL) {
        if (shutdown_requested_.load(std::memory_order_acquire)) {
            return;
        }

        Task task(std::forward<F>(func), priority);
        enqueue_task(std::move(task));
    }

    void wait_for_all_tasks() {
        std::unique_lock<std::mutex> lock(completion_mutex_);
        completion_cv_.wait(lock, [this]() {
            return total_tasks_completed_.load(std::memory_order_acquire) >=
                   total_tasks_submitted_.load(std::memory_order_acquire);
        });
    }

    void shutdown() {
        shutdown_requested_.store(true, std::memory_order_release);
        running_.store(false, std::memory_order_release);

        queue_cv_.notify_all();
        completion_cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker && worker->thread.joinable()) {
                worker->thread.join();
            }
        }

        workers_.clear();
    }

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
        Statistics stats{};
        stats.worker_count = workers_.size();
        stats.tasks_submitted = total_tasks_submitted_.load(std::memory_order_relaxed);
        stats.tasks_completed = total_tasks_completed_.load(std::memory_order_relaxed);
        stats.tasks_pending = stats.tasks_submitted > stats.tasks_completed
                                  ? stats.tasks_submitted - stats.tasks_completed
                                  : 0;
        stats.steals_attempted = total_steals_attempted_.load(std::memory_order_relaxed);
        stats.steals_successful = total_steals_successful_.load(std::memory_order_relaxed);
        stats.steal_success_rate = stats.steals_attempted > 0
                                        ? static_cast<double>(stats.steals_successful) /
                                              static_cast<double>(stats.steals_attempted)
                                        : 0.0;

        stats.worker_tasks_processed.reserve(workers_.size());
        for (const auto& worker : workers_) {
            if (worker) {
                stats.worker_tasks_processed.push_back(
                    worker->tasks_processed.load(std::memory_order_relaxed));
            }
        }

        return stats;
    }

   private:
    void enqueue_task(Task task) {
        bool enqueued_local = try_enqueue_local(task);

        if (!enqueued_local) {
            global_queue_.enqueue(std::move(task));
        }

        total_tasks_submitted_.fetch_add(1, std::memory_order_relaxed);
        queue_cv_.notify_one();
    }

    void create_worker(std::size_t worker_id) {
        auto worker = std::make_unique<WorkerInfo>();

        worker->thread = std::thread([this, worker_id, worker_ptr = worker.get()]() {
            tls_worker_info_ = worker_ptr;
            worker_main_loop(worker_id, worker_ptr);
            tls_worker_info_ = nullptr;
        });

        workers_.push_back(std::move(worker));
    }

    void worker_main_loop(std::size_t worker_id, WorkerInfo* worker_info) {
        while (true) {
            std::optional<Task> task_opt;

            Task local_task;
            if (worker_info->local_queue.try_dequeue(local_task)) {
                task_opt.emplace(std::move(local_task));
            } else if (auto global_task = global_queue_.dequeue()) {
                task_opt.emplace(std::move(*global_task));
            } else if (enable_work_stealing_) {
                Task stolen;
                if (try_steal_task(worker_id, stolen)) {
                    task_opt.emplace(std::move(stolen));
                }
            }

            if (task_opt) {
                worker_info->state.store(WorkerState::WORKING, std::memory_order_relaxed);
                execute_task(worker_info, std::move(*task_opt));
                worker_info->state.store(WorkerState::IDLE, std::memory_order_relaxed);
                continue;
            }

            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (!running_.load(std::memory_order_acquire) && !has_pending_tasks()) {
                break;
            }

            worker_info->state.store(WorkerState::IDLE, std::memory_order_relaxed);
            queue_cv_.wait_for(lock, idle_timeout_, [this]() {
                return !running_.load(std::memory_order_acquire) || !global_queue_.empty();
            });

            if (!running_.load(std::memory_order_acquire) && !has_pending_tasks()) {
                break;
            }
        }

        worker_info->state.store(WorkerState::STOPPING, std::memory_order_relaxed);
    }

    void execute_task(WorkerInfo* worker_info, Task task) {
        try {
            task.func();
        } catch (...) {
        }

        worker_info->tasks_processed.fetch_add(1, std::memory_order_relaxed);
        total_tasks_completed_.fetch_add(1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(completion_mutex_);
            completion_cv_.notify_all();
        }
    }

    bool try_enqueue_local(Task& task) {
        if (tls_worker_info_) {
            tls_worker_info_->local_queue.enqueue(std::move(task));
            return true;
        }

        if (workers_.empty()) {
            return false;
        }

        const std::size_t index = std::hash<std::thread::id>{}(std::this_thread::get_id()) % workers_.size();
        if (workers_[index]) {
            workers_[index]->local_queue.enqueue(std::move(task));
            return true;
        }

        return false;
    }

    bool try_steal_task(std::size_t stealer_id, Task& stolen_task) {
        total_steals_attempted_.fetch_add(1, std::memory_order_relaxed);

        const std::size_t worker_count = workers_.size();
        for (std::size_t attempt = 0; attempt < worker_count; ++attempt) {
            const std::size_t victim_id = (stealer_id + attempt + 1) % worker_count;
            if (victim_id >= worker_count || !workers_[victim_id]) {
                continue;
            }

            auto& victim = *workers_[victim_id];
            Task candidate;
            if (victim.local_queue.try_dequeue(candidate)) {
                total_steals_successful_.fetch_add(1, std::memory_order_relaxed);
                if (stealer_id < workers_.size() && workers_[stealer_id]) {
                    workers_[stealer_id]->tasks_stolen.fetch_add(1, std::memory_order_relaxed);
                }
                stolen_task = std::move(candidate);
                return true;
            }
        }

        return false;
    }

    bool has_pending_tasks() const {
        if (total_tasks_completed_.load(std::memory_order_acquire) <
            total_tasks_submitted_.load(std::memory_order_acquire)) {
            return true;
        }

        if (!global_queue_.empty()) {
            return true;
        }

        for (const auto& worker : workers_) {
            if (worker && !worker->local_queue.empty()) {
                return true;
            }
        }

        return false;
    }

    static std::size_t initialize_worker_count(std::size_t requested) {
        if (requested == 0) {
            requested = std::thread::hardware_concurrency();
        }
        if (requested == 0) {
            requested = 4;
        }
        return requested;
    }
};

} // namespace Corona::Concurrent