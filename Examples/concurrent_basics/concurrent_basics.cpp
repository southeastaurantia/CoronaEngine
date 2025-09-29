/**
 * Corona Concurrent Toolkit Usage Examples
 * Demonstrates basic usage of various components
 */

#include <concurrent.h>
#include <iostream>
#include <vector>
#include <thread>

using namespace Corona::Concurrent;

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
}

// Example 2: SpinLock
void spinlock_example() {
    std::cout << "\n=== SpinLock Example ===" << std::endl;
    
    SpinLock lock;
    AtomicInt32 shared_counter{0};
    std::vector<std::thread> threads;
    
    // Launch multiple threads, each thread increments the counter
    constexpr int NUM_THREADS = 4;
    constexpr int INCREMENTS_PER_THREAD = 1000;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&lock, &shared_counter]() {
            for (int j = 0; j < INCREMENTS_PER_THREAD; ++j) {
                SpinGuard guard(lock);
                int current = shared_counter.load(std::memory_order_relaxed);
                shared_counter.store(current + 1, std::memory_order_relaxed);
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Expected value: " << NUM_THREADS * INCREMENTS_PER_THREAD << std::endl;
    std::cout << "Actual value: " << shared_counter.load() << std::endl;
}

// Example 3: Read-Write Lock
void rwlock_example() {
    std::cout << "\n=== Read-Write Lock Example ===" << std::endl;
    
    RWLock rwlock;
    int shared_data = 0;
    std::vector<std::thread> threads;
    
    // Launch reader threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&rwlock, &shared_data, i]() {
            for (int j = 0; j < 5; ++j) {
                ReadGuard guard(rwlock);
                std::cout << "Reader " << i << " read: " << shared_data << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    // Launch writer thread
    threads.emplace_back([&rwlock, &shared_data]() {
        for (int i = 0; i < 5; ++i) {
            WriteGuard guard(rwlock);
            shared_data = i * 10;
            std::cout << "Writer wrote: " << shared_data << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    });
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
}

// Example 4: Memory Allocator
void allocator_example() {
    std::cout << "\n=== Memory Allocator Example ===" << std::endl;
    
    // Allocate some memory
    constexpr std::size_t alloc_size = 256;
    void* ptr1 = AllocatorManager::allocate(alloc_size, alignof(std::max_align_t));
    void* ptr2 = AllocatorManager::allocate_aligned(alloc_size);
    
    std::cout << "Allocated memory address 1: " << ptr1 << std::endl;
    std::cout << "Allocated memory address 2: " << ptr2 << std::endl;
    std::cout << "Address 2 is cache-line aligned: " << 
        (is_aligned(ptr2, CACHE_LINE_SIZE) ? "Yes" : "No") << std::endl;
    
    // Free memory
    AllocatorManager::deallocate(ptr1, alloc_size);
    AllocatorManager::deallocate(ptr2, alloc_size);
    
    // Display statistics
    std::cout << "Total allocated bytes: " << AllocatorManager::total_allocated_bytes() << std::endl;
    std::cout << "Total allocations: " << AllocatorManager::total_allocations() << std::endl;
}

// Example 5: Thread Tools
void thread_tools_example() {
    std::cout << "\n=== Thread Tools Example ===" << std::endl;
    
    // Get CPU information
    auto cpu_info = get_cpu_info();
    std::cout << "Physical cores: " << cpu_info.physical_cores << std::endl;
    std::cout << "Logical cores: " << cpu_info.logical_cores << std::endl;
    std::cout << "Hyper-threading support: " << (cpu_info.has_hyper_threading ? "Yes" : "No") << std::endl;
    
    // Get current thread ID
    auto thread_id = get_current_thread_id();
    std::cout << "Current thread ID: " << thread_id << std::endl;
    
    // High-precision timing
    HighResTimer timer;
    timer.start();
    
    // Perform some work
    volatile int sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    
    auto elapsed = timer.elapsed_micros();
    std::cout << "Computation time: " << elapsed << " microseconds" << std::endl;
}

int main() {
    std::cout << "Corona Concurrent Toolkit Example Program" << std::endl;
    std::cout << "Version: " << version.string << std::endl;
    std::cout << std::endl;
    
    // Initialize library
    initialize();
    
    try {
        // Run various examples
        atomic_example();
        spinlock_example();
        rwlock_example();
        allocator_example();
        thread_tools_example();
        
        // Display runtime statistics
        std::cout << "\n=== Runtime Statistics ===" << std::endl;
        auto stats = get_runtime_stats();
        std::cout << "Total allocated memory: " << stats.total_memory_allocated << " bytes" << std::endl;
        std::cout << "Total allocations: " << stats.total_memory_allocations << std::endl;
        std::cout << "Cache hit rate: " << stats.cache_hit_rate_percent << "%" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup library
    finalize();
    
    std::cout << "\nExample program completed successfully!" << std::endl;
    return 0;
}