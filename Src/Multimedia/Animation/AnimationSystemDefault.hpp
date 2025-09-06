//
// Created by 47226 on 2025/9/4.
//

#ifndef CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#define CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace CoronaEngine
{

    class AnimationSystemDefault final : public BaseMultimediaSystem
    {
      public:
        static AnimationSystemDefault &get_singleton();

        const char *name() override;

      protected:
        explicit AnimationSystemDefault();
        ~AnimationSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;
    };

} // namespace CoronaEngine

#endif // CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
