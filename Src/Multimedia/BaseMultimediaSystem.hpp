//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_BASEMULTIMEDIASYSTEM_HPP
#define CABBAGEFRAMEWORK_BASEMULTIMEDIASYSTEM_HPP
#include <atomic>
#include <memory>
#include <thread>

namespace CoronaEngine
{

    class BaseMultimediaSystem
    {
      public:
        using FPS = int64_t;

        explicit BaseMultimediaSystem(FPS fps = 60);

        void start();
        void stop();
        void set_fps(FPS fps);
        FPS get_fps() const;

        virtual const char *name() = 0; // name用于日志输出

      protected:
        virtual ~BaseMultimediaSystem() = default;
        virtual void _start() = 0;
        virtual void _tick() = 0;
        virtual void _stop() = 0;

      protected:
        std::atomic<FPS> max_fps;
        std::atomic<bool> engine_is_running; // 运行标志, start时置为true, stop时置为false
        std::unique_ptr<std::thread> worker;
    };

} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_BASEMULTIMEDIASYSTEM_HPP
