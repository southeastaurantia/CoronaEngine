#pragma once

#include "core/engine/ThreadedSystem.h"

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
  void process_display(uint64_t id);
    };
} // namespace Corona
