#include "ISystem.h"
#include "Events.hpp"

#include <format>
#include <iostream>

namespace ECS
{
    ISystem::ISystem()
    {
        start();
    }

    ISystem::~ISystem()
    {
        quit();
    }

    bool ISystem::isRunning() const
    {
        return this->running;
    }

    void ISystem::start()
    {
        running = true;
        mainloopThread = std::make_unique<std::thread>(&ISystem::mainloop, this);
        this->onStart();
        std::cout << std::format("[{}] started.\n", this->getName());
    }

    void ISystem::quit()
    {
        running = false;
        this->onQuit();
        if (mainloopThread != nullptr)
        {
            mainloopThread->join();
        }
        std::cout << std::format("[{}] quited.\n", this->getName());
    }
} // namespace ECS