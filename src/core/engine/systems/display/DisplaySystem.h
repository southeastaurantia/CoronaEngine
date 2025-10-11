#pragma once

#include "Core/Engine/ThreadedSystem.h"

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
        void processDisplay(uint64_t id);
    };
} // namespace Corona
