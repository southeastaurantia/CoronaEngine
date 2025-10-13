#pragma once

#include "ThreadedSystem.h"

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
  void process_audio(uint64_t id);
    };
} // namespace Corona
