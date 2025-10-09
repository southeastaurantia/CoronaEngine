#include <concurrent.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

bool run_hash_map_integrity(std::size_t thread_count, std::size_t ops_per_thread) {
    Corona::Concurrent::ConcurrentHashMap<int, int> map;
    const std::size_t total_ops = thread_count * ops_per_thread;
    std::vector<int> keys(total_ops);
    std::iota(keys.begin(), keys.end(), 0);

    std::atomic<bool> failed{false};

    // Phase 1: concurrent unique inserts
    {
        std::vector<std::thread> workers;
        workers.reserve(thread_count);
        for (std::size_t tid = 0; tid < thread_count; ++tid) {
            workers.emplace_back([&, tid]() {
                const std::size_t start = tid * ops_per_thread;
                for (std::size_t i = 0; i < ops_per_thread; ++i) {
                    const int key = keys[start + i];
                    if (!map.insert(key, key)) {
                        failed.store(true, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto& thread : workers) {
            thread.join();
        }
    }

    if (map.size() != total_ops) {
        failed.store(true, std::memory_order_relaxed);
    }

    // Phase 2: interleaved reads
    {
        std::vector<std::thread> workers;
        workers.reserve(thread_count);
        for (std::size_t tid = 0; tid < thread_count; ++tid) {
            workers.emplace_back([&, tid]() {
                for (std::size_t i = 0; i < ops_per_thread; ++i) {
                    const std::size_t index = i * thread_count + tid;
                    if (index >= keys.size()) {
                        break;
                    }
                    const int key = keys[index];
                    auto value = map.find(key);
                    if (!value.has_value() || *value != key) {
                        failed.store(true, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto& thread : workers) {
            thread.join();
        }
    }

    // Phase 3: strided erase
    {
        std::vector<std::thread> workers;
        workers.reserve(thread_count);
        for (std::size_t tid = 0; tid < thread_count; ++tid) {
            workers.emplace_back([&, tid]() {
                for (std::size_t i = tid; i < total_ops; i += thread_count) {
                    const int key = keys[i];
                    if (!map.erase(key)) {
                        failed.store(true, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto& thread : workers) {
            thread.join();
        }
    }

    if (!map.empty()) {
        failed.store(true, std::memory_order_relaxed);
    }

    return !failed.load(std::memory_order_relaxed);
}

bool run_hash_map_contention(std::size_t thread_count, std::size_t key_space, std::chrono::milliseconds duration) {
    Corona::Concurrent::ConcurrentHashMap<int, int> map;
    std::atomic<bool> failed{false};

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (std::size_t tid = 0; tid < thread_count; ++tid) {
        workers.emplace_back([&, tid]() {
            std::mt19937 rng(static_cast<unsigned int>(Clock::now().time_since_epoch().count()) + static_cast<unsigned int>(tid * 997));
            std::uniform_int_distribution<int> key_dist(0, static_cast<int>(key_space - 1));
            std::uniform_int_distribution<int> action_dist(0, 2);
            const auto deadline = Clock::now() + duration;

            while (Clock::now() < deadline) {
                const int key = key_dist(rng);
                const int action = action_dist(rng);
                switch (action) {
                    case 0:  // insert / update
                        map.insert(key, static_cast<int>(tid));
                        break;
                    case 1: {  // find
                        auto value = map.find(key);
                        if (value.has_value()) {
                            // best-effort sanity check
                            if (*value < 0) {
                                failed.store(true, std::memory_order_relaxed);
                            }
                        }
                        break;
                    }
                    case 2:  // erase
                        map.erase(key);
                        break;
                    default:
                        break;
                }
            }
        });
    }

    for (auto& thread : workers) {
        thread.join();
    }

    map.for_each([&](const int& key, const int& value) {
        auto check = map.find(key);
        if (!check.has_value() || *check != value) {
            failed.store(true, std::memory_order_relaxed);
        }
    });

    return !failed.load(std::memory_order_relaxed);
}

bool run_queue_stress(std::size_t producer_count, std::size_t consumer_count, std::size_t items_per_producer) {
    Corona::Concurrent::MPMCQueue<int> queue;
    const std::size_t expected_items = producer_count * items_per_producer;

    std::atomic<std::size_t> produced{0};
    std::atomic<std::size_t> consumed{0};
    std::atomic<bool> failed{false};

    std::vector<std::thread> workers;
    workers.reserve(producer_count + consumer_count);

    for (std::size_t pid = 0; pid < producer_count; ++pid) {
        workers.emplace_back([&, pid]() {
            const int base = static_cast<int>(pid * items_per_producer);
            for (std::size_t i = 0; i < items_per_producer; ++i) {
                queue.enqueue(base + static_cast<int>(i));
                produced.fetch_add(1, std::memory_order_release);
            }
        });
    }

    for (std::size_t cid = 0; cid < consumer_count; ++cid) {
        workers.emplace_back([&, cid]() {
            (void)cid;
            while (true) {
                auto current_consumed = consumed.load(std::memory_order_acquire);
                if (current_consumed >= expected_items) {
                    break;
                }
                if (auto item = queue.dequeue()) {
                    consumed.fetch_add(1, std::memory_order_release);
                    if (*item < 0) {
                        failed.store(true, std::memory_order_relaxed);
                    }
                } else {
                    const auto produced_total = produced.load(std::memory_order_acquire);
                    if (produced_total >= expected_items) {
                        std::this_thread::yield();
                    } else {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }

    for (auto& thread : workers) {
        thread.join();
    }

    while (auto leftover = queue.dequeue()) {
        consumed.fetch_add(1, std::memory_order_relaxed);
        if (*leftover < 0) {
            failed.store(true, std::memory_order_relaxed);
        }
    }

    if (consumed.load(std::memory_order_relaxed) != expected_items) {
        failed.store(true, std::memory_order_relaxed);
    }

    if (!queue.empty()) {
        failed.store(true, std::memory_order_relaxed);
    }

    return !failed.load(std::memory_order_relaxed);
}

bool run_thread_pool_nested(std::size_t worker_count, std::size_t primary_tasks, std::size_t nested_per_task) {
    Corona::Concurrent::ThreadPool pool(worker_count, true, 250ms);
    std::atomic<std::size_t> nested_remaining{primary_tasks * nested_per_task};
    std::atomic<bool> failed{false};

        for (std::size_t i = 0; i < primary_tasks; ++i) {
            pool.submit_detached([&, i]() {
            for (std::size_t j = 0; j < nested_per_task; ++j) {
                pool.submit_detached([&, i, j]() {
                    nested_remaining.fetch_sub(1, std::memory_order_relaxed);
                }, Corona::Concurrent::ThreadPool::Priority::LOW);
            }
        }, Corona::Concurrent::ThreadPool::Priority::HIGH);
    }

    pool.wait_for_all_tasks();
    pool.shutdown();

    if (nested_remaining.load(std::memory_order_relaxed) != 0) {
        failed.store(true, std::memory_order_relaxed);
    }

    return !failed.load(std::memory_order_relaxed);
}

bool run_thread_pool_with_containers(std::size_t worker_count, std::size_t producer_tasks, std::size_t items_per_task) {
    Corona::Concurrent::ThreadPool pool(worker_count, true, 250ms);
    Corona::Concurrent::ConcurrentHashMap<int, int> map;
    Corona::Concurrent::MPMCQueue<int> queue;

    const std::size_t expected_items = producer_tasks * items_per_task;
    std::atomic<std::size_t> produced{0};
    std::atomic<std::size_t> consumed{0};
    std::atomic<bool> failed{false};

    for (std::size_t task = 0; task < producer_tasks; ++task) {
        pool.submit_detached([&, task]() {
            const int base = static_cast<int>(task * items_per_task);
            for (std::size_t i = 0; i < items_per_task; ++i) {
                const int key = base + static_cast<int>(i);
                map.insert(key, key);
                queue.enqueue(key);
            }
            produced.fetch_add(items_per_task, std::memory_order_release);
        }, Corona::Concurrent::ThreadPool::Priority::NORMAL);
    }

    std::vector<std::thread> consumers;
    const std::size_t consumer_threads = std::max<std::size_t>(1, worker_count / 2);
    consumers.reserve(consumer_threads);

    for (std::size_t cid = 0; cid < consumer_threads; ++cid) {
        consumers.emplace_back([&, cid]() {
            (void)cid;
            while (consumed.load(std::memory_order_acquire) < expected_items) {
                if (auto item = queue.dequeue()) {
                    auto value = map.find(*item);
                    if (!value.has_value() || *value != *item) {
                        failed.store(true, std::memory_order_relaxed);
                    }
                    map.erase(*item);
                    consumed.fetch_add(1, std::memory_order_release);
                } else {
                    const auto produced_total = produced.load(std::memory_order_acquire);
                    if (produced_total >= expected_items) {
                        std::this_thread::yield();
                    } else {
                        std::this_thread::yield();
                    }
                }
            }
        });
    }

    pool.wait_for_all_tasks();
    pool.shutdown();

    for (auto& thread : consumers) {
        thread.join();
    }

    while (auto leftover = queue.dequeue()) {
        consumed.fetch_add(1, std::memory_order_relaxed);
        map.erase(*leftover);
    }

    if (!map.empty()) {
        failed.store(true, std::memory_order_relaxed);
    }

    if (consumed.load(std::memory_order_relaxed) != expected_items) {
        failed.store(true, std::memory_order_relaxed);
    }

    return !failed.load(std::memory_order_relaxed);
}

void print_status(const std::string& name, bool ok) {
    std::cout << "[" << name << "] " << (ok ? "PASS" : "FAIL") << std::endl;
}

}  // namespace

int main() {
    using namespace Corona::Concurrent;

    initialize();

    bool overall_ok = true;

    const std::vector<std::size_t> map_thread_counts{2, 4, 8};
    const std::size_t map_ops_per_thread = 1024;
    for (std::size_t threads : map_thread_counts) {
        const bool ok = run_hash_map_integrity(threads, map_ops_per_thread);
        print_status("HashMap integrity x" + std::to_string(threads), ok);
        overall_ok &= ok;
    }

    {
        const bool ok = run_hash_map_contention(6, 4096, 300ms);
        print_status("HashMap contention", ok);
        overall_ok &= ok;
    }

    {
        const bool ok = run_queue_stress(4, 4, 1500);
        print_status("MPMC queue stress", ok);
        overall_ok &= ok;
    }

    {
        const bool ok = run_thread_pool_nested(4, 8, 4);
        print_status("ThreadPool nested", ok);
        overall_ok &= ok;
    }

    {
        const bool ok = run_thread_pool_with_containers(6, 12, 256);
        print_status("ThreadPool + containers", ok);
        overall_ok &= ok;
    }

    finalize();

    if (overall_ok) {
        std::cout << "All concurrent stability tests passed." << std::endl;
        return 0;
    }

    std::cerr << "One or more concurrent stability tests failed." << std::endl;
    std::cin.get();
    return 1;
}
