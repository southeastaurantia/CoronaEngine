//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_MULTIMEDIA_H
#define CABBAGEFRAMEWORK_MULTIMEDIA_H
#include <atomic>
#include <oneapi/tbb.h>

namespace CabbageFW
{

    class BaseMultimediaSystem
    {
      public:
        BaseMultimediaSystem() = default;
        virtual ~BaseMultimediaSystem() = default;

        virtual void start() = 0;
        virtual void stop() = 0;

        virtual const char *name() = 0; // name用于日志输出

      protected:
        std::atomic<bool> engine_is_running{false}; // 运行标志, start时置为true, stop时置为false

    };

} // namespace CabbageFW

#endif // CABBAGEFRAMEWORK_MULTIMEDIA_H
