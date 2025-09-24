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
        while (!stop.load())
        {
            if (q.try_pop(v)) cons.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    p.join(); c.join();
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load() << "\n";
}

template <typename Q>
static void run_mpsc(Q &q, size_t producers, size_t seconds)
{
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
        while (!stop.load())
        {
            if (q.try_pop(v)) cons.fetch_add(1, std::memory_order_relaxed);
        }
    });
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    for (auto &t : ps) t.join();
    c.join();
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load() << "\n";
}

template <typename Q>
static void run_spmc(Q &q, size_t consumers, size_t seconds)
{
    std::atomic<bool> stop{false};
    std::atomic<size_t> prod{0};
    std::vector<std::thread> cs;
    for (size_t i = 0; i < consumers; ++i)
    {
        cs.emplace_back([&]{
            int v;
            while (!stop.load())
            {
                (void)q.try_pop(v);
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
    std::cout << "ops: prod=" << prod.load() << ", cons~=prod (期望近似)" << "\n";
}

template <typename Q>
static void run_mpmc(Q &q, size_t producers, size_t consumers, size_t seconds)
{
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
            while (!stop.load())
            {
                if (q.try_pop(v)) cons.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    stop.store(true);
    for (auto &t : ps) t.join();
    for (auto &t : cs) t.join();
    std::cout << "ops: prod=" << prod.load() << ", cons=" << cons.load() << "\n";
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
        std::cout << "MPSC Unbounded (P=4): ";
        CE::UnboundedMPSCQueue<int> q;
        run_mpsc(q, 4, 2);
    }

    // 5) SPMC 定长
    {
        std::cout << "SPMC Bounded (N=1024, C=4): ";
        CE::BoundedSPMCQueue<int, 1024> q;
        run_spmc(q, 4, 2);
    }

    // 6) SPMC 不定长
    {
        std::cout << "SPMC Unbounded (C=4): ";
        CE::UnboundedSPMCQueue<int> q;
        run_spmc(q, 4, 2);
    }

    // 7) MPMC 定长
    {
        std::cout << "MPMC Bounded (N=1024, P=4, C=4): ";
        CE::BoundedMPMCQueue<int, 1024> q;
        run_mpmc(q, 4, 4, 2);
    }

    // 8) MPMC 不定长
    {
        std::cout << "MPMC Unbounded (P=4, C=4): ";
        CE::UnboundedMPMCQueue<int> q;
        run_mpmc(q, 4, 4, 2);
    }

    std::cout << "[Lock-Free Queue Stress] End\n";
    return 0;
}
