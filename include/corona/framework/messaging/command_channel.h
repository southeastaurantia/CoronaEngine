#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace corona::framework::messaging {

struct command_channel_options {
    std::size_t max_pending_requests = 128;
};

template <typename Request, typename Response>
class command_channel {
   public:
    using request_type = Request;
    using response_type = Response;
    using handler_type = std::function<Response(const Request&)>;

    explicit command_channel(command_channel_options options = {});
    ~command_channel();

    command_channel(const command_channel&) = delete;
    command_channel& operator=(const command_channel&) = delete;

    std::future<Response> send_async(const Request& request);
    std::future<Response> send_async(Request&& request);
    Response send_sync(const Request& request);
    Response send_sync(Request&& request);

    void register_handler(handler_type handler);
    void reset_handler();

   private:
    struct queued_request {
        Request payload;
        std::promise<Response> promise;
    };

    void worker_loop();

    command_channel_options options_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<queued_request> queue_;
    handler_type handler_;
    std::thread worker_;
    bool shutting_down_ = false;
};

template <typename Request, typename Response>
command_channel<Request, Response>::command_channel(command_channel_options options)
    : options_(options) {
    if (options_.max_pending_requests == 0) {
        options_.max_pending_requests = 1;
    }
    worker_ = std::thread([this] { worker_loop(); });
}

template <typename Request, typename Response>
command_channel<Request, Response>::~command_channel() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        shutting_down_ = true;
    }
    condition_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

template <typename Request, typename Response>
std::future<Response> command_channel<Request, Response>::send_async(const Request& request) {
    return send_async(Request(request));
}

template <typename Request, typename Response>
std::future<Response> command_channel<Request, Response>::send_async(Request&& request) {
    std::promise<Response> promise;
    auto future = promise.get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() >= options_.max_pending_requests) {
            throw std::runtime_error("command queue is full");
        }
        queue_.push_back(queued_request{std::move(request), std::move(promise)});
    }
    condition_.notify_one();
    return future;
}

template <typename Request, typename Response>
Response command_channel<Request, Response>::send_sync(const Request& request) {
    return send_async(request).get();
}

template <typename Request, typename Response>
Response command_channel<Request, Response>::send_sync(Request&& request) {
    return send_async(std::move(request)).get();
}

template <typename Request, typename Response>
void command_channel<Request, Response>::register_handler(handler_type handler) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        handler_ = std::move(handler);
    }
    condition_.notify_all();
}

template <typename Request, typename Response>
void command_channel<Request, Response>::reset_handler() {
    std::lock_guard<std::mutex> lock(mutex_);
    handler_ = nullptr;
}

template <typename Request, typename Response>
void command_channel<Request, Response>::worker_loop() {
    while (true) {
        queued_request current_request;
        handler_type current_handler;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [&] { return shutting_down_ || (!queue_.empty() && handler_); });
            if (shutting_down_) {
                break;
            }
            if (queue_.empty() || !handler_) {
                continue;
            }
            current_request = std::move(queue_.front());
            queue_.pop_front();
            current_handler = handler_;
        }

        try {
            auto result = current_handler(current_request.payload);
            current_request.promise.set_value(std::move(result));
        } catch (...) {
            try {
                current_request.promise.set_exception(std::current_exception());
            } catch (...) {
                // ignore
            }
        }
    }

    // Drain remaining requests with error if shutting down
    std::deque<queued_request> remaining;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        remaining.swap(queue_);
    }
    for (auto& pending : remaining) {
        try {
            throw std::runtime_error("command channel stopped");
        } catch (...) {
            try {
                pending.promise.set_exception(std::current_exception());
            } catch (...) {
                // ignore
            }
        }
    }
}

}  // namespace corona::framework::messaging
