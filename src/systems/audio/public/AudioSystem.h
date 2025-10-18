#pragma once

#include <corona/interfaces/ThreadedSystem.h>

#include <cstdint>

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
    void process_audio(std::uint64_t id);
    };
} // namespace Corona
