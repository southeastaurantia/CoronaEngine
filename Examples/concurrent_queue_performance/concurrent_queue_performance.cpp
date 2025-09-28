#include <ConcurrentQueue.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

namespace {

double run_unbounded_benchmark(int producer_count, int consumer_count, int tasks_per_producer) {
    Corona::Concurrent::ConcurrentQueue<int> queue;
    const int total_tasks = producer_count * tasks_per_producer;
    const int sentinel = -1;

    auto start = Clock::now();

    std::vector<std::thread> consumers;
    consumers.reserve(consumer_count);
    std::atomic<int> consumed{0};
    for (int id = 0; id < consumer_count; ++id) {
        consumers.emplace_back([&queue, &consumed, sentinel]() {
            while (true) {
                int value = queue.pop();
                if (value == sentinel) {
                    queue.push(sentinel);  // 将哨兵信号重新推回队列，传递给其他消费者
                    break;                 // 然后自己退出
                }
                consumed.fetch_add(1, std::memory_order_release);
            }
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(producer_count);
    for (int id = 0; id < producer_count; ++id) {
        producers.emplace_back([&queue, tasks_per_producer]() {
            for (int i = 0; i < tasks_per_producer; ++i) {
                queue.push(i);
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    // 推送完成信号。
    for (int i = 0; i < consumer_count; ++i) {
        queue.push(sentinel);
    }

    for (auto& consumer : consumers) {
        consumer.join();
    }

    auto end = Clock::now();
    const auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double seconds = static_cast<double>(duration_ns) / 1'000'000'000.0;
    return static_cast<double>(total_tasks) / seconds;
}

double run_bounded_benchmark(int producer_count, int consumer_count, int tasks_per_producer, std::size_t capacity) {
    Corona::Concurrent::ConcurrentBoundedQueue<int> queue(capacity);
    std::atomic<int> consumed{0};
    const int total_tasks = producer_count * tasks_per_producer;
    const int sentinel = -1;

    auto start = Clock::now();

    std::vector<std::thread> consumers;
    consumers.reserve(consumer_count);
    for (int id = 0; id < consumer_count; ++id) {
        consumers.emplace_back([&queue, &consumed, sentinel]() {
            while (true) {
                int value = queue.pop();
                if (value == sentinel) {
                    queue.push(sentinel);  // 将哨兵信号重新推回队列，传递给其他消费者
                    break;                 // 然后自己退出
                }
                consumed.fetch_add(1, std::memory_order_release);
            }
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(producer_count);
    for (int id = 0; id < producer_count; ++id) {
        producers.emplace_back([&queue, tasks_per_producer]() {
            for (int i = 0; i < tasks_per_producer; ++i) {
                queue.push(i);
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    for (int i = 0; i < consumer_count; ++i) {
        queue.push(sentinel);
    }

    for (auto& consumer : consumers) {
        consumer.join();
    }

    auto end = Clock::now();
    const auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    const double seconds = static_cast<double>(duration_ns) / 1'000'000'000.0;
    return static_cast<double>(total_tasks) / seconds;
}

}  // namespace

int main() {
    const int producer_count = 4;
    const int consumer_count = 4;
    const int tasks_per_producer = 100'000;

    std::cout << "================ Concurrent queue benchmark ================\n";
    std::cout << "Producer threads: " << producer_count << ", consumer threads: " << consumer_count
              << ", tasks per producer: " << tasks_per_producer << std::endl;

    double unbounded_ops = run_unbounded_benchmark(producer_count, consumer_count, tasks_per_producer);
    std::cout << "Unbounded queue throughput ≈ " << static_cast<std::int64_t>(unbounded_ops) << " ops/s" << std::endl;

    double bounded_ops = run_bounded_benchmark(producer_count, consumer_count, tasks_per_producer, /*capacity=*/256);
    std::cout << "Bounded queue throughput ≈ " << static_cast<std::int64_t>(bounded_ops) << " ops/s" << std::endl;

    std::cout << "==================================================\n";
    std::cout << "Note: results depend on CPU, compiler optimizations, and runtime load." << std::endl;
    return 0;
}
