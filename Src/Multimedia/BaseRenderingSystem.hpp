//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_BASERENDERINGSYSTEM_HPP
#define CABBAGEFRAMEWORK_BASERENDERINGSYSTEM_HPP
#include "BaseMultimediaSystem.hpp"

namespace CoronaEngine
{
    class BaseRenderingSystem : public BaseMultimediaSystem
    {
      public:
        explicit BaseRenderingSystem(FPS fps);

        // TODO: 后续添加其他抽象方法用于拓展
        // virtual void else_abstract_method() = 0;
    };
} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_BASERENDERINGSYSTEM_HPP
