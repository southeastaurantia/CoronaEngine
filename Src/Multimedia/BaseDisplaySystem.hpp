//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_BASEDISPLAYSYSTEM_HPP
#define CABBAGEFRAMEWORK_BASEDISPLAYSYSTEM_HPP
#include "BaseMultimediaSystem.hpp"

namespace CoronaEngine
{
    class BaseDisplaySystem : public BaseMultimediaSystem
    {
      public:
        explicit BaseDisplaySystem(FPS fps);

        // TODO: 后续添加其他抽象方法用于拓展
        // virtual void else_abstract_method() = 0;
    };
} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_BASEDISPLAYSYSTEM_HPP
