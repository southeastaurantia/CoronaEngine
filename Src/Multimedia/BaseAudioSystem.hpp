//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_BASEAUDIOSYSTEM_HPP
#define CABBAGEFRAMEWORK_BASEAUDIOSYSTEM_HPP
#include "BaseMultimediaSystem.hpp"

namespace CoronaEngine
{
    class BaseAudioSystem : public BaseMultimediaSystem
    {
      public:
        explicit BaseAudioSystem(FPS fps);

        // TODO: 后续添加其他抽象方法用于拓展
        // virtual void else_abstract_method() = 0;
    };
} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_BASEAUDIOSYSTEM_HPP
