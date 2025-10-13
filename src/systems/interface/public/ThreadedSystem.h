#pragma once

#include "ISystem.h"
#include <corona_logger.h>

#include <atomic>
#include <chrono>
#include <thread>

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

        // 默认 tick 转发到 onTick，子类通常只需覆写 onTick
        void tick() override
        {
            onTick();
        }

      protected:
        // 设置目标帧率（fps<=0 时回退到 60）
        void SetTargetFps(int fps)
        {
            if (fps <= 0)
                fps = 60;
            targetFrameTimeUs_ = 1'000'000 / fps;
        }

        // 生命周期：线程启动前回调
        virtual void onStart()
        {
        }
        // 生命周期：每帧逻辑（由专用线程以固定节奏调用）
        virtual void onTick()
        {
        }
        // 生命周期：线程停止后回调
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
