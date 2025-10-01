#pragma once

#include "../core/atomic.h"
#include <vector>
#include <thread>
#include <functional>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <atomic>

namespace Corona::Concurrent::Benchmark {

/**
 * 并发性能基准测试框架
 * 
 * 提供标准化的测试环境和指标收集，用于评估并发数据结构的性能
 */
class ConcurrentBenchmark {
public:
    /**
     * 测试结果结构
     */
    struct Result {
        std::string test_name;                    // 测试名称
        size_t thread_count;                      // 线程数量
        size_t operations_per_thread;             // 每线程操作数
        size_t total_operations;                  // 总操作数
        std::chrono::microseconds duration;       // 测试耗时（微秒）
        double ops_per_second;                   // 每秒操作数
        double avg_latency_ns;                   // 平均延迟（纳秒）
        size_t contentions;                      // 竞争次数
        
        void print() const {
            std::cout << "=== " << test_name << " ===" << std::endl;
            std::cout << "Threads: " << thread_count << std::endl;
            std::cout << "Operations per thread: " << operations_per_thread << std::endl;
            std::cout << "Total operations: " << total_operations << std::endl;
            std::cout << "Duration: " << duration.count() << " us" << std::endl;
            std::cout << "Throughput: " << std::fixed << std::setprecision(0) 
                      << ops_per_second << " ops/sec" << std::endl;
            std::cout << "Average latency: " << std::fixed << std::setprecision(2) 
                      << avg_latency_ns << " ns/op" << std::endl;
            std::cout << "Contentions: " << contentions << std::endl;
            std::cout << std::endl;
        }
    };

    /**
     * 线程工作函数类型
     * 参数：线程ID，操作数量，返回竞争次数
     */
    using WorkerFunc = std::function<size_t(size_t thread_id, size_t operations)>;

    /**
     * 运行基准测试
     * @param test_name 测试名称
     * @param thread_count 线程数量
     * @param operations_per_thread 每线程操作数
     * @param worker_func 工作线程函数
     * @return 测试结果
     */
    static Result run_benchmark(
        const std::string& test_name,
        size_t thread_count,
        size_t operations_per_thread,
        WorkerFunc worker_func
    ) {
        std::vector<std::thread> threads;
        std::vector<size_t> contentions(thread_count, 0);
        
        // 使用原子计数器同步启动
        std::atomic<size_t> ready_count{0};
        std::atomic<bool> start_flag{false};
        
        // 创建工作线程
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&, i]() {
                // 报告就绪
                ready_count.fetch_add(1, std::memory_order_relaxed);
                
                // 等待启动信号
                while (!start_flag.load(std::memory_order_acquire)) {
                    std::this_thread::yield();
                }
                
                // 执行工作负载
                contentions[i] = worker_func(i, operations_per_thread);
            });
        }
        
        // 等待所有线程就绪
        while (ready_count.load(std::memory_order_relaxed) < thread_count) {
            std::this_thread::yield();
        }
        
        // 记录开始时间并发送启动信号
        auto start_time = std::chrono::high_resolution_clock::now();
        start_flag.store(true, std::memory_order_release);
        
        // 等待所有线程完成
        for (auto& t : threads) {
            t.join();
        }
        
        // 记录结束时间
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        // 计算统计信息
        size_t total_operations = thread_count * operations_per_thread;
        size_t total_contentions = 0;
        for (size_t c : contentions) {
            total_contentions += c;
        }
        
        double ops_per_second = static_cast<double>(total_operations) / 
                               (static_cast<double>(duration.count()) / 1000000.0);
        double avg_latency_ns = static_cast<double>(duration.count() * 1000) / 
                               static_cast<double>(total_operations);
        
        return Result{
            test_name,
            thread_count,
            operations_per_thread,
            total_operations,
            duration,
            ops_per_second,
            avg_latency_ns,
            total_contentions
        };
    }

    /**
     * 运行多线程缩放测试
     * @param test_name 测试名称基础名
     * @param max_threads 最大线程数
     * @param operations_per_thread 每线程操作数
     * @param worker_func 工作线程函数
     * @return 所有测试结果
     */
    static std::vector<Result> run_scaling_test(
        const std::string& test_name,
        size_t max_threads,
        size_t operations_per_thread,
        WorkerFunc worker_func
    ) {
        std::vector<Result> results;
        
        std::cout << "Running scaling test: " << test_name << std::endl;
        std::cout << "Max threads: " << max_threads << std::endl;
        std::cout << "Operations per thread: " << operations_per_thread << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        for (size_t threads = 1; threads <= max_threads; threads *= 2) {
            std::string full_name = test_name + " (" + std::to_string(threads) + " threads)";
            auto result = run_benchmark(full_name, threads, operations_per_thread, worker_func);
            result.print();
            results.push_back(result);
        }
        
        // 打印汇总表格
        print_scaling_summary(results);
        
        return results;
    }

private:
    static void print_scaling_summary(const std::vector<Result>& results) {
        std::cout << std::string(80, '=') << std::endl;
        std::cout << "SCALING SUMMARY" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << std::setw(8) << "Threads" 
                  << std::setw(12) << "Ops/Sec" 
                  << std::setw(12) << "Latency(ns)"
                  << std::setw(12) << "Speedup"
                  << std::setw(12) << "Efficiency"
                  << std::setw(12) << "Contentions" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        double baseline_ops = results.empty() ? 1.0 : results[0].ops_per_second;
        
        for (const auto& result : results) {
            double speedup = result.ops_per_second / baseline_ops;
            double efficiency = speedup / static_cast<double>(result.thread_count) * 100.0;
            
            std::cout << std::setw(8) << result.thread_count
                      << std::setw(12) << std::fixed << std::setprecision(0) << result.ops_per_second
                      << std::setw(12) << std::fixed << std::setprecision(1) << result.avg_latency_ns
                      << std::setw(12) << std::fixed << std::setprecision(2) << speedup
                      << std::setw(11) << std::fixed << std::setprecision(1) << efficiency << "%"
                      << std::setw(12) << result.contentions << std::endl;
        }
        
        std::cout << std::string(80, '=') << std::endl << std::endl;
    }
};

/**
 * 具体的并发容器基准测试
 */
namespace Tests {

/**
 * 哈希表插入性能测试
 */
template<typename HashMap>
class HashMapInsertTest {
private:
    HashMap* map_;
    Core::AtomicSize contention_counter_{0};
    
public:
    explicit HashMapInsertTest(HashMap* map) : map_(map) {}
    
    ConcurrentBenchmark::WorkerFunc get_worker() {
        return [this](size_t thread_id, size_t operations) -> size_t {
            size_t local_contentions = 0;
            size_t base_key = thread_id * 1000000;  // 避免键冲突
            
            for (size_t i = 0; i < operations; ++i) {
                size_t key = base_key + i;
                if (!map_->insert(key, static_cast<int>(key))) {
                    local_contentions++;
                }
            }
            
            return local_contentions;
        };
    }
};

/**
 * 哈希表查找性能测试
 */
template<typename HashMap>
class HashMapLookupTest {
private:
    HashMap* map_;
    std::vector<size_t> keys_;
    
public:
    HashMapLookupTest(HashMap* map, const std::vector<size_t>& keys) 
        : map_(map), keys_(keys) {}
    
    ConcurrentBenchmark::WorkerFunc get_worker() {
        return [this](size_t thread_id, size_t operations) -> size_t {
            size_t hits = 0;
            size_t key_count = keys_.size();
            
            for (size_t i = 0; i < operations; ++i) {
                size_t key = keys_[(thread_id * operations + i) % key_count];
                if (map_->find(key)) {
                    hits++;
                }
            }
            
            return hits;  // 返回命中次数作为"竞争"计数
        };
    }
};

/**
 * 队列生产者-消费者测试
 */
template<typename Queue>
class QueueProducerConsumerTest {
private:
    Queue* queue_;
    size_t producer_threads_;
    size_t consumer_threads_;
    
public:
    QueueProducerConsumerTest(Queue* queue, size_t producers, size_t consumers)
        : queue_(queue), producer_threads_(producers), consumer_threads_(consumers) {}
    
    ConcurrentBenchmark::WorkerFunc get_producer_worker() {
        return [this](size_t thread_id, size_t operations) -> size_t {
            for (size_t i = 0; i < operations; ++i) {
                int value = static_cast<int>(thread_id * 1000000 + i);
                queue_->enqueue(value);
            }
            return 0;  // 生产者无竞争
        };
    }
    
    ConcurrentBenchmark::WorkerFunc get_consumer_worker() {
        return [this](size_t thread_id, size_t operations) -> size_t {
            size_t consumed = 0;
            size_t failures = 0;
            
            while (consumed < operations) {
                auto item = queue_->dequeue();
                if (item) {
                    consumed++;
                } else {
                    failures++;
                    std::this_thread::yield();  // 避免忙等
                }
            }
            
            return failures;  // 返回失败次数
        };
    }
};

} // namespace Tests

} // namespace Corona::Concurrent::Benchmark