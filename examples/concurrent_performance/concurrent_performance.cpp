#include <concurrent.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct BenchmarkResult {
    std::string name;
    std::size_t threads{0};
    std::size_t total_ops{0};
    std::size_t errors{0};
    std::chrono::nanoseconds duration{0};

    [[nodiscard]] double throughput_ops_per_sec() const {
        if (duration.count() <= 0 || total_ops == 0) {
            return 0.0;
        }
        return static_cast<double>(total_ops) * 1'000'000'000.0 / static_cast<double>(duration.count());
    }

    [[nodiscard]] double avg_latency_ns() const {
        if (total_ops == 0) {
            return 0.0;
        }
        return static_cast<double>(duration.count()) / static_cast<double>(total_ops);
    }
};

void print_result(std::ostream& os, const BenchmarkResult& result) {
    const auto duration_ms = std::chrono::duration<double, std::milli>(result.duration).count();
    os << std::left << std::setw(28) << result.name
       << " | threads: " << std::setw(4) << result.threads
       << " | ops: " << std::setw(12) << result.total_ops
       << " | time: " << std::setw(8) << std::fixed << std::setprecision(2) << duration_ms << " ms"
       << " | throughput: " << std::setw(8) << std::setprecision(2)
       << (result.throughput_ops_per_sec() / 1'000'000.0) << " Mops/s"
       << " | latency: " << std::setw(8) << result.avg_latency_ns() << " ns/op";
    if (result.errors != 0) {
        os << " | errors: " << result.errors;
    }
    os << '\n';
}

void print_table_header(std::ostream& os) {
    os << std::left << std::setw(28) << "Test" << " | threads | ops         | time     | throughput | latency" << '\n';
}

void print_table_separator(std::ostream& os) {
    os << std::string(110, '-') << '\n';
}

inline void wait_for_start(std::atomic<std::size_t>& ready, std::size_t expected) {
    while (ready.load(std::memory_order_acquire) < expected) {
        std::this_thread::yield();
    }
}

BenchmarkResult benchmark_hashmap_insert(
    Corona::Concurrent::ConcurrentHashMap<int, int>& map,
    std::size_t thread_count,
    std::size_t ops_per_thread) {
    map.clear();
    const std::size_t total_ops = thread_count * ops_per_thread;

    std::atomic<std::size_t> ready{0};
    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (std::size_t tid = 0; tid < thread_count; ++tid) {
        workers.emplace_back([&, tid]() {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            const int base = static_cast<int>(tid * ops_per_thread);
            for (std::size_t i = 0; i < ops_per_thread; ++i) {
                const int key = base + static_cast<int>(i);
                map.insert(key, static_cast<int>(i));
            }
        });
    }

    wait_for_start(ready, thread_count);
    const auto t0 = Clock::now();
    start.store(true, std::memory_order_release);
    for (auto& t : workers) {
        t.join();
    }
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0);

    BenchmarkResult result{"HashMap Insert", thread_count, total_ops, 0, duration};
    const std::size_t current_size = map.size();
    if (current_size != total_ops) {
        result.errors = current_size > total_ops ? (current_size - total_ops) : (total_ops - current_size);
    }
    return result;
}

BenchmarkResult benchmark_hashmap_find(
    Corona::Concurrent::ConcurrentHashMap<int, int>& map,
    std::size_t thread_count,
    std::size_t ops_per_thread) {
    const std::size_t total_ops = thread_count * ops_per_thread;

    std::atomic<std::size_t> ready{0};
    std::atomic<bool> start{false};
    std::atomic<std::size_t> misses{0};
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (std::size_t tid = 0; tid < thread_count; ++tid) {
        workers.emplace_back([&, tid]() {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            const int base = static_cast<int>(tid * ops_per_thread);
            for (std::size_t i = 0; i < ops_per_thread; ++i) {
                const int key = base + static_cast<int>(i);
                auto value = map.find(key);
                if (!value) {
                    misses.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    wait_for_start(ready, thread_count);
    const auto t0 = Clock::now();
    start.store(true, std::memory_order_release);
    for (auto& t : workers) {
        t.join();
    }
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0);

    BenchmarkResult result{"HashMap Find", thread_count, total_ops, misses.load(std::memory_order_relaxed), duration};
    return result;
}

BenchmarkResult benchmark_mpmc_queue(
    std::size_t producer_threads,
    std::size_t consumer_threads,
    std::size_t items_per_producer) {
    Corona::Concurrent::MPMCQueue<int> queue;

    const std::size_t total_items = producer_threads * items_per_producer;
    const std::size_t total_ops = total_items * 2;  // enqueue + dequeue per item

    std::atomic<std::size_t> ready{0};
    std::atomic<bool> start{false};
    std::atomic<std::size_t> produced{0};
    std::atomic<std::size_t> consumed{0};

    std::vector<std::thread> workers;
    workers.reserve(producer_threads + consumer_threads);

    for (std::size_t pid = 0; pid < producer_threads; ++pid) {
        workers.emplace_back([&, pid]() {
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            const int base = static_cast<int>(pid * items_per_producer);
            std::size_t local = 0;
            for (std::size_t i = 0; i < items_per_producer; ++i) {
                queue.enqueue(base + static_cast<int>(i));
                ++local;
            }
            produced.fetch_add(local, std::memory_order_release);
        });
    }

    for (std::size_t cid = 0; cid < consumer_threads; ++cid) {
        workers.emplace_back([&, cid]() {
            (void)cid;
            ready.fetch_add(1, std::memory_order_release);
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            while (true) {
                auto already_consumed = consumed.load(std::memory_order_acquire);
                if (already_consumed >= total_items) {
                    break;
                }
                if (auto item = queue.dequeue()) {
                    (void)item;
                    consumed.fetch_add(1, std::memory_order_release);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    const std::size_t total_threads = producer_threads + consumer_threads;
    wait_for_start(ready, total_threads);

    const auto t0 = Clock::now();
    start.store(true, std::memory_order_release);
    for (auto& t : workers) {
        t.join();
    }
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0);

    BenchmarkResult result{"MPMC Queue (prod/cons)", total_threads, total_ops, 0, duration};
    const std::size_t produced_total = produced.load(std::memory_order_relaxed);
    const std::size_t consumed_total = consumed.load(std::memory_order_relaxed);
    if (produced_total != total_items) {
        result.errors += produced_total > total_items ? (produced_total - total_items) : (total_items - produced_total);
    }
    if (consumed_total != total_items) {
        result.errors += consumed_total > total_items ? (consumed_total - total_items) : (total_items - consumed_total);
    }
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace Corona::Concurrent;

    initialize();

    std::vector<std::ostream*> outputs;
    outputs.push_back(&std::cout);

    std::string output_file_path{"concurrent_performance_results.txt"};
    if (argc > 1 && argv[1] != nullptr) {
        std::string path_arg{argv[1]};
        if (!path_arg.empty()) {
            output_file_path = path_arg;
        }
    }

    std::ofstream file_stream;
    if (!output_file_path.empty()) {
        file_stream.open(output_file_path, std::ios::out | std::ios::trunc);
        if (file_stream.is_open()) {
            outputs.push_back(&file_stream);
            std::cout << "Writing performance results to '" << output_file_path << "'.\n";
        } else {
            std::cerr << "Warning: unable to open performance log file '" << output_file_path
                      << "'. Proceeding with console output only.\n";
        }
    }

    auto broadcast = [&](auto&& fn) {
        for (auto* os : outputs) {
            fn(*os);
        }
    };

    broadcast([](std::ostream& os) {
        os << "==============================\n";
        os << " Corona Concurrent Performance\n";
        os << "==============================\n";
    });

    std::vector<std::size_t> thread_counts{1, 2, 4, 8, 16, 32};
    std::sort(thread_counts.begin(), thread_counts.end());

    std::vector<std::size_t> ops_counts{5000, 10000, 20000, 30000, 40000, 50000};
    std::sort(ops_counts.begin(), ops_counts.end());

    broadcast([&](std::ostream& os) {
        os << '\n';
        os << "Thread sweep    : ";
        for (std::size_t i = 0; i < thread_counts.size(); ++i) {
            os << thread_counts[i];
            if (i + 1 < thread_counts.size()) {
                os << ", ";
            }
        }
        os << '\n';
        os << "Ops/thread sweep: ";
        for (std::size_t i = 0; i < ops_counts.size(); ++i) {
            os << ops_counts[i];
            if (i + 1 < ops_counts.size()) {
                os << ", ";
            }
        }
        os << "\n\n";
    });

    for (const auto thread_count : thread_counts) {
        for (const auto ops_count : ops_counts) {
            broadcast([&](std::ostream& os) {
                os << "[Scenario] threads=" << thread_count << ", ops_per_thread=" << ops_count << '\n';
            });
            broadcast([](std::ostream& os) {
                print_table_header(os);
            });
            broadcast([](std::ostream& os) {
                print_table_separator(os);
            });

            ConcurrentHashMap<int, int> map;
            auto insert_result = benchmark_hashmap_insert(map, thread_count, ops_count);
            auto find_result = benchmark_hashmap_find(map, thread_count, ops_count);

            broadcast([&](std::ostream& os) {
                print_result(os, insert_result);
            });
            broadcast([&](std::ostream& os) {
                print_result(os, find_result);
            });

            if (thread_count >= 2) {
                std::size_t producer_threads = std::max<std::size_t>(1, thread_count / 2);
                std::size_t consumer_threads = thread_count - producer_threads;
                if (consumer_threads == 0) {
                    consumer_threads = 1;
                    ++producer_threads;
                }
                auto queue_result = benchmark_mpmc_queue(producer_threads, consumer_threads, ops_count);
                broadcast([&](std::ostream& os) {
                    print_result(os, queue_result);
                });
                broadcast([](std::ostream& os) {
                    print_table_separator(os);
                });
                broadcast([&](std::ostream& os) {
                    os << "Queue producer threads : " << producer_threads << '\n';
                    os << "Queue consumer threads : " << consumer_threads << '\n';
                    os << "Queue total items      : " << producer_threads * ops_count << '\n';
                });
            } else {
                broadcast([](std::ostream& os) {
                    print_table_separator(os);
                    os << "MPMC Queue benchmark skipped (requires at least 2 threads)." << '\n';
                });
            }

            broadcast([](std::ostream& os) {
                os << '\n';
            });
        }
    }

    if (file_stream.is_open()) {
        file_stream.flush();
    }

    finalize();
    return 0;
}
