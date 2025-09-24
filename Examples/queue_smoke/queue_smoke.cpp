#include <iostream>
#include "Core/Thread/SafeQueue/BoundedSPSCQueue.hpp"

int main()
{
    Corona::BoundedSPSCQueue<int, 1024> q;
    q.try_push(42);
    int v = 0;
    if (q.try_pop(v))
    {
        std::cout << "popped value=" << v << "\n";
    }
    else
    {
        std::cout << "queue empty\n";
    }
    return 0;
}
