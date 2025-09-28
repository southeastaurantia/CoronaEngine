#include <ConcurrentQueue.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// 示例：展示 Corona 并发队列的典型使用方式。
int main() {
    using Corona::Concurrent::ConcurrentBoundedQueue;
    using Corona::Concurrent::ConcurrentQueue;

    std::cout << "==== Unbounded queue: multiple producers & consumers ====" << std::endl;

    ConcurrentQueue<std::string> task_queue;
    std::atomic<bool> running{true};

    // 启动两个消费者线程，通过阻塞 pop 获取任务。
    std::vector<std::thread> workers;
    for (int i = 0; i < 2; ++i) {
        workers.emplace_back([i, &task_queue, &running]() {
            std::string job;
            while (running.load(std::memory_order_acquire)) {
                if (task_queue.try_pop(job)) {
                    std::cout << "[worker " << i << "] execute: " << job << std::endl;
                } else {
                    std::this_thread::sleep_for(1ms);
                }
            }

            // 清空残留任务，保证全部处理完。
            while (task_queue.try_pop(job)) {
                std::cout << "[worker " << i << "] flush leftover: " << job << std::endl;
            }
        });
    }

    // 主线程连续推入任务。
    for (int task_id = 0; task_id < 5; ++task_id) {
        task_queue.push("job-" + std::to_string(task_id));
    }

    std::this_thread::sleep_for(10ms);
    running.store(false, std::memory_order_release);
    task_queue.abort();

    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    std::cout << '\n';
    std::cout << "==== Bounded queue: backpressure demo ====" << std::endl;

    ConcurrentBoundedQueue<int> bounded_queue(/*capacity=*/2);
    std::atomic<int> produced{0};

    std::thread consumer([&]() {
        for (int i = 0; i < 5; ++i) {
            int value = bounded_queue.pop();
            std::cout << "consume -> " << value << std::endl;
            std::this_thread::sleep_for(5ms);
        }
        bounded_queue.abort();
    });

    // try_push 失败时说明当前已满，此时可以选择等待或执行其他逻辑。
    while (produced.load() < 5) {
        int value = produced.load();
        if (bounded_queue.try_push(value)) {
            produced.fetch_add(1);
            std::cout << "produce -> " << value << std::endl;
        } else {
            std::cout << "queue full, waiting..." << std::endl;
            std::this_thread::sleep_for(2ms);
        }
    }

    if (consumer.joinable()) {
        consumer.join();
    }

    std::cout << "Demo complete." << std::endl;
    return 0;
}
