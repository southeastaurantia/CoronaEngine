// 使用示例：展示 8 种队列的基础 API 用法（push/pop/构造/容量/并发）
#include <atomic>
#include <iostream>
#include <thread>


#include "Core/Thread/SafeQueue/BoundedMPMCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedMPSCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedSPMCQueue.hpp"
#include "Core/Thread/SafeQueue/BoundedSPSCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedMPMCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedMPSCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedSPMCQueue.hpp"
#include "Core/Thread/SafeQueue/UnboundedSPSCQueue.hpp"


namespace CE = Corona;

static void demo_bounded_spsc()
{
    std::cout << "[Usage] Bounded SPSC (N=8)" << std::endl;
    CE::BoundedSPSCQueue<int, 8> q;
    std::thread p([&] { for(int i=0;i<16;++i){ while(!q.try_push(i)){} } });
    std::thread c([&] { int v; int count=0; while(count<16){ if(q.try_pop(v)){ std::cout<<v<<" "; ++count; } } std::cout<<"\n"; });
    p.join();
    c.join();
}

static void demo_bounded_mpsc()
{
    std::cout << "[Usage] Bounded MPSC (N=32, P=2)" << std::endl;
    CE::BoundedMPSCQueue<int, 32> q;
    std::atomic<int> prod{0};
    std::thread p1([&] { for(int i=0;i<100;++i){ if(q.try_push(i)) prod++; else i--; } });
    std::thread p2([&] { for(int i=100;i<200;++i){ if(q.try_push(i)) prod++; else i--; } });
    std::thread c([&] { int v,cnt=0; while(cnt<200){ if(q.try_pop(v)) { ++cnt; } } std::cout<<"total="<<cnt<<"\n"; });
    p1.join();
    p2.join();
    c.join();
}

static void demo_bounded_spmc()
{
    std::cout << "[Usage] Bounded SPMC (N=32, C=2)" << std::endl;
    CE::BoundedSPMCQueue<int, 32> q;
    std::atomic<int> cons{0};
    std::thread p([&] { for(int i=0;i<200;++i){ if(!q.try_push(i)){ --i; } } });
    std::thread c1([&] { int v; while(cons<200){ if(q.try_pop(v)) cons++; } });
    std::thread c2([&] { int v; while(cons<200){ if(q.try_pop(v)) cons++; } });
    p.join();
    c1.join();
    c2.join();
}

static void demo_bounded_mpmc()
{
    std::cout << "[Usage] Bounded MPMC (N=128, P=2, C=2)" << std::endl;
    CE::BoundedMPMCQueue<int, 128> q;
    std::atomic<int> prod{0}, cons{0};
    std::thread p1([&] { for(int i=0;i<200;++i){ if(q.try_push(i)) prod++; else i--; } });
    std::thread p2([&] { for(int i=1000;i<1200;++i){ if(q.try_push(i)) prod++; else i--; } });
    std::thread c1([&] { int v; while(cons<400){ if(q.try_pop(v)) cons++; } });
    std::thread c2([&] { int v; while(cons<400){ if(q.try_pop(v)) cons++; } });
    p1.join();
    p2.join();
    c1.join();
    c2.join();
}

static void demo_unbounded_spsc()
{
    std::cout << "[Usage] Unbounded SPSC" << std::endl;
    CE::UnboundedSPSCQueue<int> q;
    std::thread p([&] { for(int i=0;i<16;++i){ q.try_push(i); } });
    std::thread c([&] { int v; int cnt=0; while(cnt<16){ if(q.try_pop(v)){ std::cout<<v<<" "; ++cnt; } } std::cout<<"\n"; });
    p.join();
    c.join();
}

static void demo_unbounded_mpsc()
{
    std::cout << "[Usage] Unbounded MPSC (P=2)" << std::endl;
    CE::UnboundedMPSCQueue<int> q;
    std::thread p1([&] { for(int i=0;i<100;++i){ while(!q.try_push(i)){} } });
    std::thread p2([&] { for(int i=100;i<200;++i){ while(!q.try_push(i)){} } });
    std::thread c([&] { int v; int cnt=0; while(cnt<200){ if(q.try_pop(v)) ++cnt; } std::cout<<"total="<<cnt<<"\n"; });
    p1.join();
    p2.join();
    c.join();
}

static void demo_unbounded_spmc()
{
    std::cout << "[Usage] Unbounded SPMC (C=2)" << std::endl;
    CE::UnboundedSPMCQueue<int> q;
    std::atomic<int> cons{0};
    std::thread p([&] { for(int i=0;i<200;++i){ while(!q.try_push(i)){} } });
    std::thread c1([&] { int v; while(cons<200){ if(q.try_pop(v)) ++cons; else std::this_thread::yield(); } });
    std::thread c2([&] { int v; while(cons<200){ if(q.try_pop(v)) ++cons; else std::this_thread::yield(); } });
    p.join();
    c1.join();
    c2.join();
}

static void demo_unbounded_mpmc()
{
    std::cout << "[Usage] Unbounded MPMC (P=2,C=2)" << std::endl;
    CE::UnboundedMPMCQueue<int> q;
    std::atomic<int> cons{0};
    std::thread p1([&] { for(int i=0;i<200;++i){ while(!q.try_push(i)){} } });
    std::thread p2([&] { for(int i=100;i<300;++i){ while(!q.try_push(i)){} } });
    std::thread c1([&] { int v; while(cons<400){ if(q.try_pop(v)) ++cons; else std::this_thread::yield(); } });
    std::thread c2([&] { int v; while(cons<400){ if(q.try_pop(v)) ++cons; else std::this_thread::yield(); } });
    p1.join();
    p2.join();
    c1.join();
    c2.join();
}

int main()
{
    demo_bounded_spsc();
    demo_bounded_mpsc();
    demo_bounded_spmc();
    demo_bounded_mpmc();
    demo_unbounded_spsc();
    demo_unbounded_mpsc();
    demo_unbounded_spmc();
    demo_unbounded_mpmc();
    return 0;
}
