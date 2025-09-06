//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_AUDIOSYSTEMDEFAULT_HPP
#define CORONAENGINE_AUDIOSYSTEMDEFAULT_HPP
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace CoronaEngine
{

    class AudioSystemDefault final : public BaseMultimediaSystem
    {
      public:
        static AudioSystemDefault &get_singleton();

        const char *name() override;

      protected:
        explicit AudioSystemDefault();
        ~AudioSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;
    };

} // namespace CoronaEngine

#endif // CORONAENGINE_AUDIOSYSTEMDEFAULT_HPP
