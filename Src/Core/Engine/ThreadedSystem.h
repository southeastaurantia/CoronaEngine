// A reusable base for threaded systems that mirrors the old Multimedia systems' pattern
#pragma once

#include <atomic>
#include <chrono>
#include <thread>

#include "Core/Engine/ISystem.h"
#include "Core/Log.h"

namespace Corona
{
    class ThreadedSystem : public ISystem
    {
      public:
        explicit ThreadedSystem(const char *sysName, int targetFps = 120)
            : name_(sysName), running_(false)
        {
            SetTargetFps(targetFps);
        }

        ~ThreadedSystem() override = default;

        const char *name() override
        {
            return name_;
        }

        void start() override
        {
            if (running_.exchange(true))
                return;
            onStart();
            worker_ = std::thread([this] {
                while (running_.load(std::memory_order_relaxed))
                {
                    const auto begin = std::chrono::high_resolution_clock::now();
                    tick();
                    const auto end = std::chrono::high_resolution_clock::now();
                    const auto frameTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
                    const auto waitTimeUs = targetFrameTimeUs_ - frameTimeUs;
                    if (waitTimeUs > 0)
                        std::this_thread::sleep_for(std::chrono::microseconds(waitTimeUs));
                }
            });
            CE_LOG_DEBUG("{} started", name());
        }

        void stop() override
        {
            if (!running_.exchange(false))
                return;
            if (worker_.joinable())
                worker_.join();
            onStop();
            CE_LOG_DEBUG("{} stopped", name());
        }

        // Default tick dispatches to onTick so subclasses only override onTick
        void tick() override
        {
            onTick();
        }

      protected:
        void SetTargetFps(int fps)
        {
            if (fps <= 0)
                fps = 60;
            targetFrameTimeUs_ = 1'000'000 / fps;
        }

        virtual void onStart()
        {
        }
        virtual void onTick()
        {
        }
        virtual void onStop()
        {
        }

      private:
        const char *name_;
        std::thread worker_{};
        std::atomic<bool> running_;
        int64_t targetFrameTimeUs_ = 1'000'000 / 120;
    };
} // namespace Corona
