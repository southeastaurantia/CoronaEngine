//
// Created by 47226 on 2025/9/4.
//

#pragma once
#include <atomic>
#include <memory>
#include <thread>

namespace Corona
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

} // namespace Corona

