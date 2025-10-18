#pragma once

#include <corona/interfaces/ThreadedSystem.h>

#include <cstdint>

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
    void process_display(std::uint64_t id);
    };
} // namespace Corona
