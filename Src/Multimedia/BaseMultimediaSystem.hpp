//
// Created by 47226 on 2025/9/4.
//

#ifndef CORONAENGINE_BASEMULTIMEDIASYSTEM_HPP
#define CORONAENGINE_BASEMULTIMEDIASYSTEM_HPP
#include <atomic>
#include <memory>
#include <thread>

namespace CoronaEngine
{

    class BaseMultimediaSystem
    {
      protected:
        explicit BaseMultimediaSystem() = default;
        virtual ~BaseMultimediaSystem() = default;

      public:
        virtual const char *name() = 0; // name用于日志输出

        virtual void start() = 0;
        virtual void tick() = 0;
        virtual void stop() = 0;
    };

} // namespace CoronaEngine

#endif // CORONAENGINE_BASEMULTIMEDIASYSTEM_HPP
