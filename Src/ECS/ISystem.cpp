#include "ISystem.h"
#include "Events.hpp"

#include <format>
#include <iostream>

namespace ECS
{
    bool ISystem::isRunning() const
    {
        return this->running;
    }

    void ISystem::registerEvents(entt::dispatcher &dispatcher)
    {
        dispatcher.sink<ECS::Events::SceneCreate>().connect<&ISystem::start>(this);
        dispatcher.sink<ECS::Events::SceneDestroy>().connect<&ISystem::quit>(this);
        this->onRegisterEvents(dispatcher);
        std::cout << std::format("[{}] registered events finished.\n", this->getName());
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