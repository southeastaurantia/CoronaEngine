//
// Created by 47226 on 2025/9/5.
//

#ifndef CABBAGEFRAMEWORK_AUDIOSYSTEMDEFAULT_HPP
#define CABBAGEFRAMEWORK_AUDIOSYSTEMDEFAULT_HPP
#include "Multimedia/BaseAudioSystem.hpp"

namespace CabbageFW
{

    class AudioSystemDefault final : public BaseAudioSystem
    {
      public:
        static AudioSystemDefault& get_singleton();

        const char *name() override;

      protected:
        explicit AudioSystemDefault(FPS fps);
        ~AudioSystemDefault() override;

        void _start() override;
        void _tick() override;
        void _stop() override;
    };

} // namespace CabbageFW

#endif // CABBAGEFRAMEWORK_AUDIOSYSTEMDEFAULT_HPP
