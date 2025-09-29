#include <concurrent.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include <iomanip>

using namespace Corona::Concurrent;

// 测试配置
constexpr size_t THREADS = 8;
constexpr size_t OPS_PER_THREAD = 10000;
constexpr size_t TOTAL_OPS = THREADS * OPS_PER_THREAD;

struct TestResult {
    std::string config_name;
    size_t shard_count;
    std::string optimization_level;
    double insert_ops_per_sec;
    double lookup_ops_per_sec;
    double insert_latency_ns;
    double lookup_latency_ns;
};

// 性能测试函数
template<typename HashMap>
TestResult run_performance_test(const std::string& config_name, HashMap& map) {
    std::cout << "\n=== 测试配置: " << config_name << " ===" << std::endl;
    
    // 获取分片信息
    auto sharding_info = map.get_sharding_info();
    std::cout << "分片数量: " << sharding_info.shard_count 
              << ", 优化级别: " << sharding_info.optimization_level << std::endl;
    
    std::vector<std::thread> threads;
    
    // 插入性能测试
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < THREADS; ++i) {
        threads.emplace_back([&map, i]() {
            size_t base_key = i * 100000;  // 避免键冲突
            
            for (size_t j = 0; j < OPS_PER_THREAD; ++j) {
                int key = static_cast<int>(base_key + j);
                std::string value = "value_" + std::to_string(key);
                map.insert(key, value);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();
    
    auto insert_end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::microseconds>(insert_end - start).count();
    double insert_ops_per_sec = static_cast<double>(TOTAL_OPS) / insert_duration * 1'000'000;
    double insert_latency_ns = static_cast<double>(insert_duration * 1000) / TOTAL_OPS;
    
    // 查找性能测试
    start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < THREADS; ++i) {
        threads.emplace_back([&map, i]() {
            size_t base_key = i * 100000;
            
            for (size_t j = 0; j < OPS_PER_THREAD; ++j) {
                int key = static_cast<int>(base_key + j);
                auto result = map.find(key);
                // 消费结果避免优化掉
                if (result.has_value()) {
                    volatile auto& val = result.value();
                    (void)val;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto lookup_end = std::chrono::high_resolution_clock::now();
    auto lookup_duration = std::chrono::duration_cast<std::chrono::microseconds>(lookup_end - start).count();
    double lookup_ops_per_sec = static_cast<double>(TOTAL_OPS) / lookup_duration * 1'000'000;
    double lookup_latency_ns = static_cast<double>(lookup_duration * 1000) / TOTAL_OPS;
    
    std::cout << "插入: " << std::fixed << std::setprecision(0) << insert_ops_per_sec << " ops/sec, "
              << std::setprecision(1) << insert_latency_ns << " ns/op" << std::endl;
    std::cout << "查找: " << std::setprecision(0) << lookup_ops_per_sec << " ops/sec, "
              << std::setprecision(1) << lookup_latency_ns << " ns/op" << std::endl;
    
    return TestResult{
        config_name,
        sharding_info.shard_count,
        sharding_info.optimization_level,
        insert_ops_per_sec,
        lookup_ops_per_sec,
        insert_latency_ns,
        lookup_latency_ns
    };
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  ConcurrentHashMap 分片配置性能对比" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "测试配置:" << std::endl;
    std::cout << "  线程数: " << THREADS << std::endl;
    std::cout << "  每线程操作数: " << OPS_PER_THREAD << std::endl;
    std::cout << "  总操作数: " << TOTAL_OPS << std::endl;
    std::cout << "  CPU核心数: " << std::thread::hardware_concurrency() << std::endl;
    
    std::vector<TestResult> results;
    
    // 测试1: 传统固定32分片配置
    {
        ConcurrentHashMap<int, std::string> map_legacy(32, 16);
        results.push_back(run_performance_test("传统固定配置 (32分片)", map_legacy));
    }
    
    // 测试2: 智能默认配置 (Balanced)
    {
        ConcurrentHashMap<int, std::string> map_default;
        results.push_back(run_performance_test("智能默认配置 (自动)", map_default));
    }
    
    // 测试3: 低延迟配置
    {
        auto map_low_latency = ConcurrentHashMap<int, std::string>::create_optimized(2);
        results.push_back(run_performance_test("低延迟优化配置", map_low_latency));
    }
    
    // 测试4: 高并发配置
    {
        auto map_high_concurrency = ConcurrentHashMap<int, std::string>::create_optimized(1);
        results.push_back(run_performance_test("高并发优化配置", map_high_concurrency));
    }
    
    // 输出对比结果
    std::cout << "\n" << std::string(100, '=') << std::endl;
    std::cout << "                           性能对比报告" << std::endl;
    std::cout << std::string(100, '=') << std::endl;
    
    // 表头
    std::cout << std::left 
              << std::setw(20) << "配置名称"
              << std::setw(8) << "分片数"
              << std::setw(15) << "优化级别"
              << std::setw(12) << "插入(ops/s)"
              << std::setw(12) << "查找(ops/s)"
              << std::setw(12) << "插入延迟(ns)"
              << std::setw(12) << "查找延迟(ns)" << std::endl;
    std::cout << std::string(100, '-') << std::endl;
    
    // 找到最佳性能作为基准
    double best_insert = 0, best_lookup = 0;
    for (const auto& result : results) {
        best_insert = std::max(best_insert, result.insert_ops_per_sec);
        best_lookup = std::max(best_lookup, result.lookup_ops_per_sec);
    }
    
    // 输出每个配置的结果
    for (const auto& result : results) {
        double insert_ratio = result.insert_ops_per_sec / best_insert;
        double lookup_ratio = result.lookup_ops_per_sec / best_lookup;
        
        std::cout << std::left << std::fixed
                  << std::setw(20) << result.config_name
                  << std::setw(8) << result.shard_count
                  << std::setw(15) << result.optimization_level
                  << std::setw(12) << std::setprecision(0) << result.insert_ops_per_sec
                  << std::setw(12) << std::setprecision(0) << result.lookup_ops_per_sec  
                  << std::setw(12) << std::setprecision(1) << result.insert_latency_ns
                  << std::setw(12) << std::setprecision(1) << result.lookup_latency_ns << std::endl;
        
        std::cout << "  性能比例: 插入 " << std::setprecision(1) << insert_ratio * 100 << "%, "
                  << "查找 " << lookup_ratio * 100 << "%" << std::endl << std::endl;
    }
    
    std::cout << std::string(100, '=') << std::endl;
    std::cout << "测试完成！" << std::endl;
    
    return 0;
}