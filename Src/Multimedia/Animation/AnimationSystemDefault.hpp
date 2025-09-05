//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_ANIMATIONSYSTEMDEFAULT_HPP
#define CABBAGEFRAMEWORK_ANIMATIONSYSTEMDEFAULT_HPP
#include "Multimedia/BaseAnimationSystem.hpp"

namespace CoronaEngine
{

    class AnimationSystemDefault final : public BaseAnimationSystem
    {
      public:
        static AnimationSystemDefault &get_singleton();

        const char *name() override;

      protected:
        explicit AnimationSystemDefault(FPS fps);
        ~AnimationSystemDefault() override;

        void _start() override;
        void _tick() override;
        void _stop() override;
    };

} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_ANIMATIONSYSTEMDEFAULT_HPP
