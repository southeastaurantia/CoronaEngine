#pragma once

#include "Core/Engine/ThreadedSystem.h"
#include <unordered_set>


namespace Corona
{
    class DisplaySystem final : public ThreadedSystem
    {
      public:
        DisplaySystem();

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        std::unordered_set<uint64_t> data_keys_{};
        void processDisplay(uint64_t id);
    };
} // namespace Corona
