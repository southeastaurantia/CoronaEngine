//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_BASEANIMATIONSYSTEM_HPP
#define CABBAGEFRAMEWORK_BASEANIMATIONSYSTEM_HPP
#include "BaseMultimediaSystem.hpp"

namespace CoronaEngine
{
    class BaseAnimationSystem : public BaseMultimediaSystem
    {
      public:
        explicit BaseAnimationSystem(FPS fps = 60);

        // TODO: 后续添加其他抽象方法用于拓展
        // virtual void else_abstract_method() = 0;
    };
} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_BASEANIMATIONSYSTEM_HPP
