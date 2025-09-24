// 简单并发压力测试：对 8 种无锁队列做短时压测（非严格基准，仅做冒烟 & 粗略吞吐观察）
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

#include "Core/Thread/SafeQueue/BoundedSPSCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedSPSCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedMPSCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedMPSCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedSPMCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedSPMCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedMPMCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedMPMCQueue.hpp"

using namespace std::chrono_literals;

namespace CE = Corona;

template <typename Q>
static void run_spsc_fixed(Q &q, size_t seconds)
{
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<bool> stop{false};
    std::atomic<size_t> prod{0}, cons{0};
    std::thread p([&]{
        int x = 0;
        while (!stop.load())
        {
            if (q.try_push(x++)) prod.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::thread c([&]{
        int v;
        while (!stop.load() || !q.empty())
        {
            if (!q.try_pop(v)) { std::this_thread::yield(); }
            else { cons.fetch_add(1, std::memory_order_relaxed); }
        }
    });
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    p.join(); c.join();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double secs = ms / 1000.0;
    double thr = secs > 0 ? cons.load() / secs : 0.0;
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load()
              << ", elapsed=" << ms << " ms, throughput=" << static_cast<uint64_t>(thr) << " ops/s\n";
}

template <typename Q>
static void run_mpsc(Q &q, size_t producers, size_t seconds)
{
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<bool> stop{false};
    std::atomic<size_t> prod{0}, cons{0};
    std::vector<std::thread> ps;
    for (size_t i = 0; i < producers; ++i)
    {
        ps.emplace_back([&]{
            int x = 0;
            while (!stop.load())
            {
                if (q.try_push(x++)) prod.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    std::thread c([&]{
        int v;
        // 在结束后继续清空队列，避免析构期长时间清理
        while (!stop.load() || !q.empty())
        {
            if (!q.try_pop(v)) { std::this_thread::yield(); }
            else { cons.fetch_add(1, std::memory_order_relaxed); }
        }
    });
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    for (auto &t : ps) t.join();
    c.join();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double secs = ms / 1000.0;
    double thr = secs > 0 ? cons.load() / secs : 0.0;
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load()
              << ", elapsed=" << ms << " ms, throughput=" << static_cast<uint64_t>(thr) << " ops/s\n";
}

template <typename Q>
static void run_spmc(Q &q, size_t consumers, size_t seconds)
{
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<bool> stop{false};
    std::atomic<size_t> prod{0}, cons{0};
    std::vector<std::thread> cs;
    for (size_t i = 0; i < consumers; ++i)
    {
        cs.emplace_back([&]{
            int v;
            while (!stop.load() || !q.empty())
            {
                if (!q.try_pop(v)) { std::this_thread::yield(); }
                else { cons.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }
    std::thread p([&]{
        int x = 0;
        while (!stop.load())
        {
            if (q.try_push(x++)) prod.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    p.join();
    for (auto &t : cs) t.join();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double secs = ms / 1000.0;
    double thr = secs > 0 ? cons.load() / secs : 0.0;
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load()
              << ", elapsed=" << ms << " ms, throughput=" << static_cast<uint64_t>(thr) << " ops/s (期望接近)\n";
}

template <typename Q>
static void run_mpmc(Q &q, size_t producers, size_t consumers, size_t seconds)
{
    auto t0 = std::chrono::steady_clock::now();
    std::atomic<bool> stop{false};
    std::atomic<size_t> prod{0}, cons{0};
    std::vector<std::thread> ps, cs;
    for (size_t i = 0; i < producers; ++i)
    {
        ps.emplace_back([&]{
            int x = 0;
            while (!stop.load())
            {
                if (q.try_push(x++)) prod.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (size_t i = 0; i < consumers; ++i)
    {
        cs.emplace_back([&]{
            int v;
            while (!stop.load() || !q.empty())
            {
                if (!q.try_pop(v)) { std::this_thread::yield(); }
                else { cons.fetch_add(1, std::memory_order_relaxed); }
            }
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    for (auto &t : ps) t.join();
    for (auto &t : cs) t.join();
    auto t1 = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    double secs = ms / 1000.0;
    double thr = secs > 0 ? cons.load() / secs : 0.0;
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load()
              << ", elapsed=" << ms << " ms, throughput=" << static_cast<uint64_t>(thr) << " ops/s\n";
}

int main()
{
    std::cout << "[Lock-Free Queue Stress] Begin\n";

    // 1) SPSC 定长
    {
        std::cout << "SPSC Bounded (N=1024): ";
        CE::BoundedSPSCQueue<int, 1024> q;
        run_spsc_fixed(q, 2);
    }

    // 2) SPSC 不定长
    {
        std::cout << "SPSC Unbounded: ";
        CE::UnboundedSPSCQueue<int> q;
        run_spsc_fixed(q, 2);
    }

    // 3) MPSC 定长
    {
        std::cout << "MPSC Bounded (N=1024, P=4): ";
        CE::BoundedMPSCQueue<int, 1024> q;
        run_mpsc(q, 4, 2);
    }

    // 4) MPSC 不定长
    {
        std::cout << "MPSC Unbounded (P=2): ";
        CE::UnboundedMPSCQueue<int> q;
        // 缩短时长并减少生产者，避免产生过大积压
        run_mpsc(q, 2, 1);
    }

    // 5) SPMC 定长
    {
        std::cout << "SPMC Bounded (N=1024, C=3): ";
        CE::BoundedSPMCQueue<int, 1024> q;
        run_spmc(q, 3, 2);
    }

    // 6) SPMC 不定长
    {
        std::cout << "SPMC Unbounded (C=3): ";
        CE::UnboundedSPMCQueue<int> q;
        run_spmc(q, 3, 2);
    }

    // 7) MPMC 定长
    {
        std::cout << "MPMC Bounded (N=1024, P=3, C=3): ";
        CE::BoundedMPMCQueue<int, 1024> q;
        run_mpmc(q, 3, 3, 2);
    }

    // 8) MPMC 不定长
    {
        std::cout << "MPMC Unbounded (P=3, C=3): ";
        CE::UnboundedMPMCQueue<int> q;
        run_mpmc(q, 3, 3, 2);
    }

    std::cout << "[Lock-Free Queue Stress] End\n";
    return 0;
}
