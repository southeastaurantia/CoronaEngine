//
// Created by 47226 on 2025/9/5.
//

#ifndef CABBAGEFRAMEWORK_DISPLAYSYSTEMDEFAULT_HPP
#define CABBAGEFRAMEWORK_DISPLAYSYSTEMDEFAULT_HPP
#include "Multimedia/BaseDisplaySystem.hpp"

namespace CoronaEngine
{

    class DisplaySystemDefault final : public BaseDisplaySystem
    {
      public:
        static DisplaySystemDefault& get_singleton();

        const char *name() override;

      protected:
        explicit DisplaySystemDefault(FPS fps);
        ~DisplaySystemDefault() override;

        void _start() override;
        void _tick() override;
        void _stop() override;
    };

} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_DISPLAYSYSTEMDEFAULT_HPP
