#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace corona::framework::services::time {
class time_service;
}

namespace corona::framework::runtime {

class worker_control;

namespace detail {

struct worker_record {
    std::string name;
    std::chrono::milliseconds tick_interval{16};
    std::function<void(worker_control&)> task;
    std::atomic<bool> stop_requested{false};
    std::mutex sleep_mutex;
    std::condition_variable sleep_condition;
    std::thread thread;
    std::exception_ptr exception;
    std::shared_ptr<corona::framework::services::time::time_service> time_source;
};

}  // namespace detail

class worker_control {
   public:
    worker_control(std::shared_ptr<detail::worker_record> record,
                   std::shared_ptr<corona::framework::services::time::time_service> time_source);

    bool should_stop() const noexcept;
    void sleep_for(std::chrono::milliseconds duration);
    void sleep_until(std::chrono::steady_clock::time_point deadline);
    void request_stop();

    [[nodiscard]] std::shared_ptr<corona::framework::services::time::time_service> time_source() const noexcept;

   private:
    std::weak_ptr<detail::worker_record> record_;
    std::weak_ptr<corona::framework::services::time::time_service> time_source_;
};

class thread_orchestrator {
   public:
    struct worker_options {
        std::chrono::milliseconds tick_interval{16};
    };

    class worker_handle {
       public:
        worker_handle() = default;
        worker_handle(const worker_handle&) = delete;
        worker_handle& operator=(const worker_handle&) = delete;
        worker_handle(worker_handle&& other) noexcept;
        worker_handle& operator=(worker_handle&& other) noexcept;
        ~worker_handle();

        bool valid() const noexcept;
        void stop();
        std::exception_ptr last_exception() const noexcept;

       private:
        friend class thread_orchestrator;
        worker_handle(thread_orchestrator* owner, std::shared_ptr<detail::worker_record> record);

        void reset();

        thread_orchestrator* owner_ = nullptr;
        std::shared_ptr<detail::worker_record> record_;
    };

    thread_orchestrator();
    explicit thread_orchestrator(std::shared_ptr<corona::framework::services::time::time_service> time_service);
    ~thread_orchestrator();

    worker_handle add_worker(std::string name,
                             worker_options options,
                             std::function<void(worker_control&)> task);

    void stop_all();

    void set_time_service(std::shared_ptr<corona::framework::services::time::time_service> time_service) noexcept;
    [[nodiscard]] std::shared_ptr<corona::framework::services::time::time_service> time_service() const noexcept;

   private:
    static void worker_loop(const std::shared_ptr<detail::worker_record>& record);

    void release_worker(const std::shared_ptr<detail::worker_record>& record);

    mutable std::mutex workers_mutex_;
    std::vector<std::shared_ptr<detail::worker_record>> workers_;
    std::shared_ptr<corona::framework::services::time::time_service> time_service_;
};

}  // namespace corona::framework::runtime