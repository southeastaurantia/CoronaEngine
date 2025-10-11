#pragma once

#include "Core/Engine/ThreadedSystem.h"

namespace Corona
{
    class AudioSystem final : public ThreadedSystem
    {
      public:
        AudioSystem();

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        void processAudio(uint64_t id);
    };
} // namespace Corona
