// 无锁队列正确性测试（轻量级），覆盖 8 种实现的基本功能与边界
#include <cassert>
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

#include "Core/Thread/SafeQueue/BoundedSPSCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedSPSCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedMPSCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedMPSCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedSPMCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedSPMCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedMPMCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedMPMCQueue.hpp"

namespace CE = Corona;

// 1) SPSC：生产 N，消费 N，校验顺序与计数
static void test_spsc()
{
    {
        CE::BoundedSPSCQueue<int, 64> q;
        constexpr int N = 100000;
        std::atomic<int> consumed{0};
        std::thread p([&]{ for (int i = 0; i < N; ++i) { while(!q.try_push(i)) {} } });
        std::thread c([&]{ int v; int last=-1; for (;;) { if (q.try_pop(v)) { assert(v==last+1); last=v; consumed++; if (consumed==N) break; } } });
        p.join(); c.join();
        assert(consumed.load() == N);
    }
    {
        CE::UnboundedSPSCQueue<int> q;
        constexpr int N = 100000;
        std::atomic<int> consumed{0};
        std::thread p([&]{ for (int i = 0; i < N; ++i) { while(!q.try_push(i)) {} } });
        std::thread c([&]{ int v; int last=-1; for (;;) { if (q.try_pop(v)) { assert(v==last+1); last=v; consumed++; if (consumed==N) break; } } });
        p.join(); c.join();
        assert(consumed.load() == N);
    }
}

// 2) MPSC：多生产，单消费，计数一致
static void test_mpsc()
{
    {
        CE::BoundedMPSCQueue<int, 256> q;
        constexpr int P = 4, N = 25000;
        std::atomic<int> produced{0}, consumed{0};
        std::vector<std::thread> ps;
        for (int i=0;i<P;++i) ps.emplace_back([&]{ for (int k=0;k<N;++k){ if(q.try_push(k)) produced++; else k--; } });
        std::thread c([&]{ int v; while(consumed < P*N){ if(q.try_pop(v)) consumed++; } });
        for (auto &t:ps) t.join(); c.join();
        assert(produced.load()==P*N && consumed.load()==P*N);
    }
    {
        CE::UnboundedMPSCQueue<int> q;
        constexpr int P = 4, N = 25000;
        std::atomic<int> produced{0}, consumed{0};
        std::vector<std::thread> ps;
        for (int i=0;i<P;++i) ps.emplace_back([&]{ for (int k=0;k<N;++k){ if(q.try_push(k)) produced++; else k--; } });
        std::thread c([&]{ int v; while(consumed < P*N){ if(q.try_pop(v)) consumed++; } });
        for (auto &t:ps) t.join(); c.join();
        assert(produced.load()==P*N && consumed.load()==P*N);
    }
}

// 3) SPMC：单生产，多消费，计数一致
static void test_spmc()
{
    {
        CE::BoundedSPMCQueue<int, 256> q;
        constexpr int C = 4, N = 25000;
        std::atomic<int> produced{0}, consumed{0};
        std::vector<std::thread> cs;
        for (int i=0;i<C;++i) cs.emplace_back([&]{ int v; while(consumed < N){ if(q.try_pop(v)) consumed++; } });
        std::thread p([&]{ for (int k=0;k<N;++k){ if(q.try_push(k)) produced++; else k--; } });
        p.join(); for (auto &t:cs) t.join();
        assert(produced.load()==N && consumed.load()==N);
    }
    {
        CE::UnboundedSPMCQueue<int> q;
        constexpr int C = 4, N = 25000;
        std::atomic<int> produced{0}, consumed{0};
        std::vector<std::thread> cs;
        for (int i=0;i<C;++i) cs.emplace_back([&]{ int v; while(consumed < N){ if(q.try_pop(v)) consumed++; } });
        std::thread p([&]{ for (int k=0;k<N;++k){ if(q.try_push(k)) produced++; else k--; } });
        p.join(); for (auto &t:cs) t.join();
        assert(produced.load()==N && consumed.load()==N);
    }
}

// 4) MPMC：多生产多消费，计数一致
static void test_mpmc()
{
    {
        CE::BoundedMPMCQueue<int, 1024> q;
        constexpr int P = 4, C = 4, N = 20000;
        std::atomic<int> produced{0}, consumed{0};
        std::vector<std::thread> ps, cs;
        for (int i=0;i<P;++i) ps.emplace_back([&]{ for (int k=0;k<N;++k){ if(q.try_push(k)) produced++; else k--; } });
        for (int i=0;i<C;++i) cs.emplace_back([&]{ int v; while(consumed < P*N){ if(q.try_pop(v)) consumed++; } });
        for (auto &t:ps) t.join(); for (auto &t:cs) t.join();
        assert(produced.load()==P*N && consumed.load()==P*N);
    }
    {
        CE::UnboundedMPMCQueue<int> q;
        constexpr int P = 4, C = 4, N = 20000;
        std::atomic<int> produced{0}, consumed{0};
        std::vector<std::thread> ps, cs;
        for (int i=0;i<P;++i) ps.emplace_back([&]{ for (int k=0;k<N;++k){ if(q.try_push(k)) produced++; else k--; } });
        for (int i=0;i<C;++i) cs.emplace_back([&]{ int v; while(consumed < P*N){ if(q.try_pop(v)) consumed++; } });
        for (auto &t:ps) t.join(); for (auto &t:cs) t.join();
        assert(produced.load()==P*N && consumed.load()==P*N);
    }
}

int main()
{
    std::cout << "[LFQ Correctness] Start" << std::endl;
    test_spsc();
    test_mpsc();
    test_spmc();
    test_mpmc();
    std::cout << "[LFQ Correctness] All tests passed" << std::endl;
    return 0;
}
