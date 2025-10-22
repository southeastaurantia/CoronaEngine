#include "corona/framework/runtime/thread_orchestrator.h"

#include <algorithm>
#include <stdexcept>

namespace corona::framework::runtime {

worker_control::worker_control(std::shared_ptr<detail::worker_record> record)
    : record_(std::move(record)) {}

bool worker_control::should_stop() const noexcept {
    if (auto record = record_.lock()) {
        return record->stop_requested.load(std::memory_order_acquire);
    }
    return true;
}

void worker_control::sleep_for(std::chrono::milliseconds duration) {
    sleep_until(std::chrono::steady_clock::now() + duration);
}

void worker_control::sleep_until(std::chrono::steady_clock::time_point deadline) {
    auto record = record_.lock();
    if (!record) {
        std::this_thread::sleep_until(deadline);
        return;
    }
    std::unique_lock<std::mutex> lock(record->sleep_mutex);
    record->sleep_condition.wait_until(lock, deadline, [&] {
        return record->stop_requested.load(std::memory_order_acquire) || std::chrono::steady_clock::now() >= deadline;
    });
}

void worker_control::request_stop() {
    if (auto record = record_.lock()) {
        record->stop_requested.store(true, std::memory_order_release);
        record->sleep_condition.notify_all();
    }
}

thread_orchestrator::worker_handle::worker_handle(thread_orchestrator* owner,
                                                  std::shared_ptr<detail::worker_record> record)
    : owner_(owner), record_(std::move(record)) {}

thread_orchestrator::worker_handle::worker_handle(worker_handle&& other) noexcept
    : owner_(other.owner_), record_(std::move(other.record_)) {
    other.owner_ = nullptr;
}

thread_orchestrator::worker_handle& thread_orchestrator::worker_handle::operator=(worker_handle&& other) noexcept {
    if (this != &other) {
        stop();
        owner_ = other.owner_;
        record_ = std::move(other.record_);
        other.owner_ = nullptr;
    }
    return *this;
}

thread_orchestrator::worker_handle::~worker_handle() {
    stop();
}

bool thread_orchestrator::worker_handle::valid() const noexcept {
    return static_cast<bool>(record_);
}

void thread_orchestrator::worker_handle::stop() {
    if (!record_) {
        return;
    }
    record_->stop_requested.store(true, std::memory_order_release);
    record_->sleep_condition.notify_all();
    if (record_->thread.joinable()) {
        record_->thread.join();
    }
    if (owner_) {
        owner_->release_worker(record_);
    }
    reset();
}

std::exception_ptr thread_orchestrator::worker_handle::last_exception() const noexcept {
    if (!record_) {
        return nullptr;
    }
    return record_->exception;
}

void thread_orchestrator::worker_handle::reset() {
    record_.reset();
    owner_ = nullptr;
}

thread_orchestrator::thread_orchestrator() = default;

thread_orchestrator::~thread_orchestrator() {
    stop_all();
}

thread_orchestrator::worker_handle thread_orchestrator::add_worker(
    std::string name,
    worker_options options,
    std::function<void(worker_control&)> task) {
    if (!task) {
        throw std::invalid_argument("thread_orchestrator requires a valid task");
    }
    if (options.tick_interval.count() <= 0) {
        options.tick_interval = std::chrono::milliseconds(1);
    }

    auto record = std::make_shared<detail::worker_record>();
    record->name = std::move(name);
    record->tick_interval = options.tick_interval;
    record->task = std::move(task);

    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        workers_.push_back(record);
    }

    record->thread = std::thread(&thread_orchestrator::worker_loop, record);

    return worker_handle(this, record);
}

void thread_orchestrator::stop_all() {
    std::vector<std::shared_ptr<detail::worker_record>> snapshot;
    {
        std::lock_guard<std::mutex> lock(workers_mutex_);
        snapshot = workers_;
        workers_.clear();
    }

    for (auto& record : snapshot) {
        record->stop_requested.store(true, std::memory_order_release);
        record->sleep_condition.notify_all();
    }

    for (auto& record : snapshot) {
        if (record->thread.joinable()) {
            record->thread.join();
        }
    }
}

void thread_orchestrator::worker_loop(const std::shared_ptr<detail::worker_record>& record) {
    worker_control control(record);
    auto next_tick = std::chrono::steady_clock::now();
    while (!control.should_stop()) {
        try {
            record->task(control);
        } catch (...) {
            record->exception = std::current_exception();
            control.request_stop();
        }
        if (record->tick_interval.count() <= 0) {
            continue;
        }
        next_tick += record->tick_interval;
        control.sleep_until(next_tick);
    }
}

void thread_orchestrator::release_worker(const std::shared_ptr<detail::worker_record>& record) {
    std::lock_guard<std::mutex> lock(workers_mutex_);
    auto it = std::find(workers_.begin(), workers_.end(), record);
    if (it != workers_.end()) {
        workers_.erase(it);
    }
}

}  // namespace corona::framework::runtime
