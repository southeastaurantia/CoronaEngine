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
        bool isRunning() const;


        void start();
        void quit();

      protected:
        virtual void onStart() = 0;
        virtual void onQuit() = 0;
        virtual void mainloop() = 0;

      private:
        bool running{false};
        std::unique_ptr<std::thread> mainloopThread;

      public:
        entt::dispatcher dispatcher{};
    };
} // namespace ECS