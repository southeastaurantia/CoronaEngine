//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
#define CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace Corona
{

    class DisplaySystemDefault final : public BaseMultimediaSystem
    {
      public:
        static DisplaySystemDefault &inst();

        const char *name() override;

      protected:
        explicit DisplaySystemDefault();
        ~DisplaySystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;
    };

} // namespace CoronaEngine

#endif // CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
