#include <concurrent.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <random>
#include <mutex>
#include <algorithm>
#include <iomanip>

using namespace Corona::Concurrent;

// Thread-safe output utility (重用之前的实现)
class ThreadSafeOutput {
private:
    static std::mutex output_mutex;
    
public:
    template<typename... Args>
    static void println(Args&&... args) {
        std::lock_guard<std::mutex> lock(output_mutex);
        ((std::cout << args), ...);
        std::cout << std::endl;
    }
};

std::mutex ThreadSafeOutput::output_mutex;

/**
 * 测试并发哈希表性能
 */
void test_concurrent_hashmap() {
    std::cout << "=== ConcurrentHashMap Performance Test ===" << std::endl;
    
    ConcurrentHashMap<int, std::string> map;
    constexpr size_t NUM_THREADS = 8;
    constexpr size_t OPERATIONS_PER_THREAD = 50000;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Threads: " << NUM_THREADS << std::endl;
    std::cout << "  Operations per thread: " << OPERATIONS_PER_THREAD << std::endl;
    std::cout << "  Total operations: " << NUM_THREADS * OPERATIONS_PER_THREAD << std::endl;
    
    // 准备测试数据
    std::vector<std::thread> threads;
    std::atomic<size_t> insert_success{0};
    std::atomic<size_t> lookup_success{0};
    
    Core::HighResTimer timer;
    timer.start();
    
    // 阶段1：并发插入测试
    std::cout << "\nPhase 1: Concurrent Insert Test" << std::endl;
    
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&map, &insert_success, i, OPERATIONS_PER_THREAD]() {
            size_t local_success = 0;
            size_t base_key = i * 100000;  // 避免键冲突
            
            for (size_t j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int key = static_cast<int>(base_key + j);
                std::string value = "value_" + std::to_string(key);
                
                if (map.insert(key, value)) {
                    local_success++;
                }
            }
            
            insert_success.fetch_add(local_success, std::memory_order_relaxed);
            ThreadSafeOutput::println("Thread ", i, " inserted ", local_success, " items");
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();
    
    auto insert_time = timer.elapsed_micros();
    
    // 显示分片信息
    auto sharding_info = map.get_sharding_info();
    std::cout << "\nSharding Information:" << std::endl;
    std::cout << "  Shard count: " << sharding_info.shard_count << std::endl;
    std::cout << "  CPU cores: " << sharding_info.cpu_cores << std::endl;
    std::cout << "  Optimization level: " << sharding_info.optimization_level << std::endl;
    std::cout << "  Buckets per shard: " << sharding_info.bucket_count_per_shard << std::endl;
    std::cout << "  Load factor: " << std::fixed << std::setprecision(3) << sharding_info.load_factor << std::endl;
    
    std::cout << "\nInsert Results:" << std::endl;
    std::cout << "  Successful inserts: " << insert_success.load() << std::endl;
    std::cout << "  Map size: " << map.size() << std::endl;
    std::cout << "  Insert time: " << insert_time << " μs" << std::endl;
    std::cout << "  Insert throughput: " << std::fixed << std::setprecision(0)
              << (static_cast<double>(insert_success.load()) / insert_time * 1000000) 
              << " ops/sec" << std::endl;
    
    // 阶段2：并发查找测试
    std::cout << "\nPhase 2: Concurrent Lookup Test" << std::endl;
    
    timer.start();
    
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&map, &lookup_success, i, OPERATIONS_PER_THREAD]() {
            size_t local_success = 0;
            std::mt19937 rng(static_cast<unsigned>(i));
            std::uniform_int_distribution<int> dist(0, static_cast<int>(NUM_THREADS * OPERATIONS_PER_THREAD - 1));
            
            for (size_t j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                int key = dist(rng);
                if (map.find(key)) {
                    local_success++;
                }
            }
            
            lookup_success.fetch_add(local_success, std::memory_order_relaxed);
            ThreadSafeOutput::println("Thread ", i, " found ", local_success, " items");
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto lookup_time = timer.elapsed_micros();
    
    std::cout << "Lookup Results:" << std::endl;
    std::cout << "  Successful lookups: " << lookup_success.load() << std::endl;
    std::cout << "  Lookup time: " << lookup_time << " μs" << std::endl;
    std::cout << "  Lookup throughput: " << std::fixed << std::setprecision(0)
              << (static_cast<double>(NUM_THREADS * OPERATIONS_PER_THREAD) / lookup_time * 1000000) 
              << " ops/sec" << std::endl;
}

/**
 * 测试MPMC队列性能
 */
void test_mpmc_queue() {
    std::cout << "\n=== MPMC Queue Performance Test ===" << std::endl;
    
    MPMCQueue<int> queue;
    constexpr size_t NUM_PRODUCERS = 4;
    constexpr size_t NUM_CONSUMERS = 4;
    constexpr size_t ITEMS_PER_PRODUCER = 25000;
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Producer threads: " << NUM_PRODUCERS << std::endl;
    std::cout << "  Consumer threads: " << NUM_CONSUMERS << std::endl;
    std::cout << "  Items per producer: " << ITEMS_PER_PRODUCER << std::endl;
    std::cout << "  Total items: " << NUM_PRODUCERS * ITEMS_PER_PRODUCER << std::endl;
    
    std::vector<std::thread> producers, consumers;
    std::atomic<size_t> produced{0};
    std::atomic<size_t> consumed{0};
    std::atomic<bool> production_done{false};
    
    Core::HighResTimer timer;
    timer.start();
    
    // 启动生产者线程
    for (size_t i = 0; i < NUM_PRODUCERS; ++i) {
        producers.emplace_back([&queue, &produced, i, ITEMS_PER_PRODUCER]() {
            size_t base_value = i * 100000;
            
            for (size_t j = 0; j < ITEMS_PER_PRODUCER; ++j) {
                int value = static_cast<int>(base_value + j);
                queue.enqueue(value);
                produced.fetch_add(1, std::memory_order_relaxed);
            }
            
            ThreadSafeOutput::println("Producer ", i, " finished producing ", ITEMS_PER_PRODUCER, " items");
        });
    }
    
    // 启动消费者线程
    for (size_t i = 0; i < NUM_CONSUMERS; ++i) {
        consumers.emplace_back([&queue, &consumed, &production_done, i]() {
            size_t local_consumed = 0;
            size_t empty_attempts = 0;
            
            while (!production_done.load(std::memory_order_acquire) || !queue.empty()) {
                auto item = queue.dequeue();
                if (item) {
                    local_consumed++;
                    consumed.fetch_add(1, std::memory_order_relaxed);
                } else {
                    empty_attempts++;
                    if (empty_attempts > 1000) {
                        std::this_thread::yield();
                        empty_attempts = 0;
                    }
                }
            }
            
            ThreadSafeOutput::println("Consumer ", i, " consumed ", local_consumed, " items");
        });
    }
    
    // 等待所有生产者完成
    for (auto& p : producers) {
        p.join();
    }
    
    production_done.store(true, std::memory_order_release);
    
    // 等待所有消费者完成
    for (auto& c : consumers) {
        c.join();
    }
    
    auto total_time = timer.elapsed_micros();
    
    std::cout << "\nQueue Test Results:" << std::endl;
    std::cout << "  Total produced: " << produced.load() << std::endl;
    std::cout << "  Total consumed: " << consumed.load() << std::endl;
    std::cout << "  Final queue size: " << queue.size() << std::endl;
    std::cout << "  Total time: " << total_time << " μs" << std::endl;
    std::cout << "  Overall throughput: " << std::fixed << std::setprecision(0)
              << (static_cast<double>(consumed.load()) / total_time * 1000000) 
              << " ops/sec" << std::endl;
}

/**
 * 运行基准测试框架演示
 */
void test_benchmark_framework() {
    std::cout << "\n=== Benchmark Framework Demo ===" << std::endl;
    
    using namespace Corona::Concurrent::Benchmark;
    
    // 测试并发哈希表的缩放性能
    ConcurrentHashMap<size_t, int> test_map;
    
    auto worker_func = [&test_map](size_t thread_id, size_t operations) -> size_t {
        size_t failures = 0;
        size_t base_key = thread_id * 1000000;
        
        for (size_t i = 0; i < operations; ++i) {
            size_t key = base_key + i;
            if (!test_map.insert(key, static_cast<int>(key))) {
                failures++;
            }
        }
        
        return failures;
    };
    
    // 运行缩放测试
    auto cpu_info = Core::get_cpu_info();
    size_t max_threads = std::min(static_cast<size_t>(8), static_cast<size_t>(cpu_info.logical_cores));
    
    ConcurrentBenchmark::run_scaling_test(
        "HashMap Insert Scaling",
        max_threads,
        10000,  // 每线程操作数
        worker_func
    );
}

/**
 * 哈希函数性能对比测试
 */
void test_hash_functions() {
    std::cout << "\n=== Hash Function Performance Test ===" << std::endl;
    
    using namespace Corona::Concurrent::Util;
    
    // 准备测试数据
    std::vector<std::string> test_strings;
    test_strings.reserve(10000);
    
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> char_dist('a', 'z');
    std::uniform_int_distribution<int> len_dist(5, 50);
    
    for (int i = 0; i < 10000; ++i) {
        std::string str;
        int len = len_dist(rng);
        str.reserve(len);
        
        for (int j = 0; j < len; ++j) {
            str += static_cast<char>(char_dist(rng));
        }
        test_strings.push_back(std::move(str));
    }
    
    std::cout << "Generated " << test_strings.size() << " test strings" << std::endl;
    
    // 测试不同哈希函数的性能
    Core::HighResTimer timer;
    
    // xxHash 64测试
    timer.start();
    std::uint64_t xxhash_checksum = 0;
    for (const auto& str : test_strings) {
        xxhash_checksum ^= Hash::xxHash::hash64(str.data(), str.size());
    }
    auto xxhash_time = timer.elapsed_micros();
    
    // FNV 64测试
    timer.start();
    std::uint64_t fnv_checksum = 0;
    for (const auto& str : test_strings) {
        fnv_checksum ^= Hash::FNV::hash64(str.data(), str.size());
    }
    auto fnv_time = timer.elapsed_micros();
    
    // Murmur 32测试
    timer.start();
    std::uint32_t murmur_checksum = 0;
    for (const auto& str : test_strings) {
        murmur_checksum ^= Hash::Murmur::hash32(str.data(), str.size());
    }
    auto murmur_time = timer.elapsed_micros();
    
    std::cout << "Hash Function Performance Results:" << std::endl;
    std::cout << "  xxHash64: " << xxhash_time << " μs, checksum: 0x" 
              << std::hex << xxhash_checksum << std::dec << std::endl;
    std::cout << "  FNV64: " << fnv_time << " μs, checksum: 0x" 
              << std::hex << fnv_checksum << std::dec << std::endl;
    std::cout << "  Murmur32: " << murmur_time << " μs, checksum: 0x" 
              << std::hex << murmur_checksum << std::dec << std::endl;
    
    std::cout << "\nRelative Performance:" << std::endl;
    double baseline = static_cast<double>(xxhash_time);
    std::cout << "  xxHash64: 1.00x (baseline)" << std::endl;
    std::cout << "  FNV64: " << std::fixed << std::setprecision(2) 
              << (static_cast<double>(fnv_time) / baseline) << "x" << std::endl;
    std::cout << "  Murmur32: " << std::fixed << std::setprecision(2) 
              << (static_cast<double>(murmur_time) / baseline) << "x" << std::endl;
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "  Corona Concurrent Containers Test Program    " << std::endl;
    std::cout << "              Version: " << version.string << "              " << std::endl;
    std::cout << "================================================" << std::endl;
    
    // 初始化并发库
    std::cout << "Initializing concurrent toolkit..." << std::endl;
    initialize();
    
    Core::HighResTimer total_timer;
    total_timer.start();
    
    try {
        // 运行各种测试
        test_concurrent_hashmap();
        test_mpmc_queue();
        test_benchmark_framework();
        test_hash_functions();
        
        // 显示最终统计
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "=== Final System Statistics ===" << std::endl;
        
        auto stats = get_runtime_stats();
        std::cout << "Runtime Statistics:" << std::endl;
        std::cout << "  Total allocated memory: " << stats.total_memory_allocated << " bytes" << std::endl;
        std::cout << "  Total allocations: " << stats.total_memory_allocations << std::endl;
        std::cout << "  Active threads: " << stats.active_threads << std::endl;
        
        auto cpu_info = Core::get_cpu_info();
        std::cout << "\nSystem Information:" << std::endl;
        std::cout << "  Physical cores: " << cpu_info.physical_cores << std::endl;
        std::cout << "  Logical cores: " << cpu_info.logical_cores << std::endl;
        std::cout << "  NUMA nodes: " << cpu_info.numa_nodes << std::endl;
        
        auto total_elapsed = total_timer.elapsed_millis();
        std::cout << "\nTotal execution time: " << total_elapsed << " milliseconds" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n*** EXCEPTION OCCURRED ***" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n*** UNKNOWN EXCEPTION OCCURRED ***" << std::endl;
        return 2;
    }
    
    // 清理并发库
    std::cout << "\nCleaning up concurrent toolkit..." << std::endl;
    finalize();
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "🎉 Concurrent containers test completed! 🎉" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    return 0;
}