//
// Created by 47226 on 2025/9/4.
//

#ifndef CABBAGEFRAMEWORK_ANIMATIONSYSTEMDEFAULT_HPP
#define CABBAGEFRAMEWORK_ANIMATIONSYSTEMDEFAULT_HPP
#include "Multimedia/BaseAnimationSystem.hpp"

namespace CabbageFW
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

} // namespace CabbageFW

#endif // CABBAGEFRAMEWORK_ANIMATIONSYSTEMDEFAULT_HPP
