#pragma once

#include <entt/entt.hpp>

#include <thread>

namespace ECS
{
    class ISystem
    {
      public:
        ISystem();
        virtual ~ISystem();

        virtual const char *getName() const = 0; // Debug used

      public:
        void registerEvents(entt::dispatcher &dispatcher); // First called
        bool isRunning() const;

      private:
        void start();
        void quit();

      protected:
        virtual void onRegisterEvents(entt::dispatcher &dispatcher) = 0;
        virtual void onStart() = 0;
        virtual void onQuit() = 0;
        virtual void mainloop() = 0;

      private:
        bool running{false};
        std::unique_ptr<std::thread> mainloopThread;
    };
} // namespace ECS