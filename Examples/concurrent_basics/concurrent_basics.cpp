/**
 * Corona Concurrent Toolkit Usage Examples
 * Demonstrates basic usage of various components
 */

#include <concurrent.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <iomanip>

using namespace Corona::Concurrent;

// Thread-safe output utility
class ThreadSafeOutput {
private:
    static std::mutex output_mutex;
    
public:
    template<typename... Args>
    static void print(Args&&... args) {
        std::lock_guard<std::mutex> lock(output_mutex);
        ((std::cout << args), ...);
    }
    
    template<typename... Args>
    static void println(Args&&... args) {
        std::lock_guard<std::mutex> lock(output_mutex);
        ((std::cout << args), ...);
        std::cout << std::endl;
    }
};

std::mutex ThreadSafeOutput::output_mutex;

// Example 1: Atomic Operations
void atomic_example() {
    std::cout << "=== Atomic Operations Example ===" << std::endl;
    
    AtomicInt64 counter{0};
    AtomicPtr<int> ptr{nullptr};
    
    // Basic operations
    counter.store(100);
    std::cout << "Counter value: " << counter.load() << std::endl;
    
    // Atomic increment
    auto old_val = counter++;
    std::cout << "Before increment: " << old_val << ", After increment: " << counter.load() << std::endl;
    
    // CAS operation
    std::int64_t expected = 101;
    if (counter.compare_exchange_strong(expected, 200)) {
        std::cout << "CAS succeeded, new value: " << counter.load() << std::endl;
    }
    
    // Test fetch_add operation
    auto prev = counter.fetch_add(50);
    std::cout << "fetch_add(50): previous=" << prev << ", current=" << counter.load() << std::endl;
    
    // Test atomic pointer operations
    int value = 42;
    AtomicPtr<int> atomic_ptr{&value};
    int* loaded = atomic_ptr.load();
    std::cout << "Atomic pointer loaded value: " << (loaded ? *loaded : 0) << std::endl;
}

// Example 2: SpinLock
void spinlock_example() {
    std::cout << "\n=== SpinLock Example ===" << std::endl;
    
    SpinLock lock;
    AtomicInt32 shared_counter{0};
    AtomicInt64 contention_count{0};
    std::vector<std::thread> threads;
    
    // Launch multiple threads, each thread increments the counter
    constexpr int NUM_THREADS = 4;
    constexpr int INCREMENTS_PER_THREAD = 10000;
    
    HighResTimer timer;
    timer.start();
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&lock, &shared_counter, &contention_count, i, INCREMENTS_PER_THREAD]() {
            for (int j = 0; j < INCREMENTS_PER_THREAD; ++j) {
                // Try lock first to measure contention
                if (!lock.try_lock()) {
                    contention_count++;
                    lock.lock();
                }
                
                int current = shared_counter.load(std::memory_order_relaxed);
                shared_counter.store(current + 1, std::memory_order_relaxed);
                
                lock.unlock();
            }
            ThreadSafeOutput::println("Thread ", i, " completed ", INCREMENTS_PER_THREAD, " increments");
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    auto elapsed = timer.elapsed_micros();
    
    std::cout << "Expected value: " << NUM_THREADS * INCREMENTS_PER_THREAD << std::endl;
    std::cout << "Actual value: " << shared_counter.load() << std::endl;
    std::cout << "Execution time: " << elapsed << " microseconds" << std::endl;
    std::cout << "Contention events: " << contention_count.load() << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) 
              << (static_cast<double>(NUM_THREADS * INCREMENTS_PER_THREAD) / elapsed * 1000000) 
              << " ops/second" << std::endl;
}

// Example 3: Read-Write Lock
void rwlock_example() {
    std::cout << "\n=== Read-Write Lock Example ===" << std::endl;
    
    RWLock rwlock;
    int shared_data = 0;
    AtomicInt32 read_count{0};
    AtomicInt32 write_count{0};
    std::vector<std::thread> threads;
    
    HighResTimer timer;
    timer.start();
    
    // Launch reader threads
    constexpr int NUM_READERS = 3;
    constexpr int READS_PER_THREAD = 8;
    
    for (int i = 0; i < NUM_READERS; ++i) {
        threads.emplace_back([&rwlock, &shared_data, &read_count, i, READS_PER_THREAD]() {
            for (int j = 0; j < READS_PER_THREAD; ++j) {
                {
                    ReadGuard guard(rwlock);
                    int data = shared_data;
                    read_count++;
                    ThreadSafeOutput::println("Reader ", i, " read: ", data, 
                                            " (read #", read_count.load(), ")");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            ThreadSafeOutput::println("Reader ", i, " finished");
        });
    }
    
    // Launch writer threads
    constexpr int NUM_WRITERS = 2;
    constexpr int WRITES_PER_THREAD = 3;
    
    for (int i = 0; i < NUM_WRITERS; ++i) {
        threads.emplace_back([&rwlock, &shared_data, &write_count, i, WRITES_PER_THREAD]() {
            for (int j = 0; j < WRITES_PER_THREAD; ++j) {
                {
                    WriteGuard guard(rwlock);
                    shared_data = (i + 1) * 10 + j;
                    write_count++;
                    ThreadSafeOutput::println("Writer ", i, " wrote: ", shared_data,
                                            " (write #", write_count.load(), ")");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
            ThreadSafeOutput::println("Writer ", i, " finished");
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    auto elapsed = timer.elapsed_micros();
    std::cout << "Final shared_data value: " << shared_data << std::endl;
    std::cout << "Total reads: " << read_count.load() << std::endl;
    std::cout << "Total writes: " << write_count.load() << std::endl;
    std::cout << "RWLock test completed in " << elapsed << " microseconds" << std::endl;
}

// Example 4: Memory Allocator
void allocator_example() {
    std::cout << "\n=== Memory Allocator Example ===" << std::endl;
    
    // Record initial statistics
    auto initial_bytes = AllocatorManager::total_allocated_bytes();
    auto initial_allocs = AllocatorManager::total_allocations();
    
    std::cout << "Initial - Allocated bytes: " << initial_bytes 
              << ", Allocations: " << initial_allocs << std::endl;
    
    std::vector<void*> pointers;
    std::vector<std::size_t> sizes;
    
    // Test different allocation sizes
    const std::vector<std::size_t> test_sizes = {32, 64, 128, 256, 512, 1024};
    
    std::cout << "\nAllocating memory blocks:" << std::endl;
    for (std::size_t size : test_sizes) {
        void* ptr1 = AllocatorManager::allocate(size, alignof(std::max_align_t));
        void* ptr2 = AllocatorManager::allocate_aligned(size);
        
        pointers.push_back(ptr1);
        pointers.push_back(ptr2);
        sizes.push_back(size);
        sizes.push_back(size);
        
        std::cout << "  Size " << std::setw(4) << size << ": "
                  << "regular=" << ptr1 
                  << ", aligned=" << ptr2 
                  << " (aligned: " << (is_aligned(ptr2, CACHE_LINE_SIZE) ? "Yes" : "No") << ")" 
                  << std::endl;
    }
    
    // Check current statistics
    auto current_bytes = AllocatorManager::total_allocated_bytes();
    auto current_allocs = AllocatorManager::total_allocations();
    
    std::cout << "\nAfter allocation - Allocated bytes: " << current_bytes 
              << ", Allocations: " << current_allocs << std::endl;
    std::cout << "Delta - Bytes: " << (current_bytes - initial_bytes) 
              << ", Allocations: " << (current_allocs - initial_allocs) << std::endl;
    
    // Free all memory
    std::cout << "\nFreeing memory blocks..." << std::endl;
    for (size_t i = 0; i < pointers.size(); ++i) {
        AllocatorManager::deallocate(pointers[i], sizes[i]);
    }
    
    // Final statistics
    auto final_bytes = AllocatorManager::total_allocated_bytes();
    auto final_allocs = AllocatorManager::total_allocations();
    
    std::cout << "Final - Allocated bytes: " << final_bytes 
              << ", Allocations: " << final_allocs << std::endl;
}

// Example 5: Thread Tools
void thread_tools_example() {
    std::cout << "\n=== Thread Tools Example ===" << std::endl;
    
    // Get CPU information
    auto cpu_info = get_cpu_info();
    std::cout << "CPU Information:" << std::endl;
    std::cout << "  Physical cores: " << cpu_info.physical_cores << std::endl;
    std::cout << "  Logical cores: " << cpu_info.logical_cores << std::endl;
    std::cout << "  NUMA nodes: " << cpu_info.numa_nodes << std::endl;
    std::cout << "  Hyper-threading: " << (cpu_info.has_hyper_threading ? "Enabled" : "Disabled") << std::endl;
    
    // Get current thread ID
    auto thread_id = get_current_thread_id();
    std::cout << "\nThread Information:" << std::endl;
    std::cout << "  Current thread ID: " << thread_id << std::endl;
    
    // Test CPU affinity (if available)
    auto affinity = CpuAffinity::get_current_affinity();
    std::cout << "  CPU affinity: ";
    for (size_t i = 0; i < affinity.size() && i < 8; ++i) {
        std::cout << affinity[i];
        if (i < affinity.size() - 1 && i < 7) std::cout << ", ";
    }
    if (affinity.size() > 8) std::cout << " ... (" << affinity.size() << " total)";
    std::cout << std::endl;
    
    // Performance benchmarks
    std::cout << "\nPerformance Benchmarks:" << std::endl;
    
    // Test 1: Simple computation
    HighResTimer timer;
    timer.start();
    volatile uint64_t sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    auto elapsed1 = timer.elapsed_micros();
    std::cout << "  Simple computation (1M ops): " << elapsed1 << " μs" << std::endl;
    
    // Test 2: Memory allocation
    timer.start();
    std::vector<void*> ptrs;
    for (int i = 0; i < 1000; ++i) {
        ptrs.push_back(AllocatorManager::allocate(64, 8));
    }
    for (void* ptr : ptrs) {
        AllocatorManager::deallocate(ptr, 64);
    }
    auto elapsed2 = timer.elapsed_micros();
    std::cout << "  Memory allocation (1K ops): " << elapsed2 << " μs" << std::endl;
    
    // Test 3: Atomic operations
    AtomicInt64 atomic_counter{0};
    timer.start();
    for (int i = 0; i < 1000000; ++i) {
        atomic_counter++;
    }
    auto elapsed3 = timer.elapsed_micros();
    std::cout << "  Atomic operations (1M ops): " << elapsed3 << " μs" << std::endl;
    std::cout << "  Final counter value: " << atomic_counter.load() << std::endl;
}

int main() {
    std::cout << "================================================" << std::endl;
    std::cout << "    Corona Concurrent Toolkit Example Program    " << std::endl;
    std::cout << "              Version: " << version.string << "              " << std::endl;
    std::cout << "================================================" << std::endl;
    
    // Initialize library
    std::cout << "Initializing concurrent toolkit..." << std::endl;
    initialize();
    
    HighResTimer total_timer;
    total_timer.start();
    
    try {
        // Run various examples
        atomic_example();
        spinlock_example();
        rwlock_example();
        allocator_example();
        thread_tools_example();
        
        // Display runtime statistics
        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "=== Final Runtime Statistics ===" << std::endl;
        
        auto stats = get_runtime_stats();
        std::cout << "Memory Statistics:" << std::endl;
        std::cout << "  Total allocated memory: " << stats.total_memory_allocated << " bytes" << std::endl;
        std::cout << "  Total allocations: " << stats.total_memory_allocations << std::endl;
        std::cout << "  Active threads: " << stats.active_threads << std::endl;
        
        auto& thread_stats = ThreadLocal::get_stats();
        std::cout << "\nThread Local Statistics:" << std::endl;
        std::cout << "  Operations performed: " << thread_stats.operations_count << std::endl;
        std::cout << "  Cache hits: " << thread_stats.cache_hits << std::endl;
        std::cout << "  Cache misses: " << thread_stats.cache_misses << std::endl;
        std::cout << "  Contentions: " << thread_stats.contentions << std::endl;
        
        if (thread_stats.cache_hits + thread_stats.cache_misses > 0) {
            double hit_rate = thread_stats.hit_rate() * 100.0;
            std::cout << "  Cache hit rate: " << std::fixed << std::setprecision(1) << hit_rate << "%" << std::endl;
        }
        
        auto total_elapsed = total_timer.elapsed_millis();
        std::cout << "\nTotal execution time: " << total_elapsed << " milliseconds" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n*** EXCEPTION OCCURRED ***" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "The program will now terminate." << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n*** UNKNOWN EXCEPTION OCCURRED ***" << std::endl;
        std::cerr << "The program will now terminate." << std::endl;
        return 2;
    }
    
    // Cleanup library
    std::cout << "\nCleaning up concurrent toolkit..." << std::endl;
    finalize();
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "🎉 Example program completed successfully! 🎉" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    return 0;
}