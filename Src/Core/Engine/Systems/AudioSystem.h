#pragma once

#include "Core/Engine/ThreadedSystem.h"
#include <unordered_set>


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
        std::unordered_set<uint64_t> data_keys_{};
        void processAudio(uint64_t id);
    };
} // namespace Corona
