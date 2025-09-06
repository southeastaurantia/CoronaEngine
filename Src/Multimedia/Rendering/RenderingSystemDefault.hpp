//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_RENDERSYSTEMDEFAULT_HPP
#define CORONAENGINE_RENDERSYSTEMDEFAULT_HPP
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace CoronaEngine
{

    class RenderingSystemDefault final : public BaseMultimediaSystem
    {
      public:
        static RenderingSystemDefault &get_singleton();

        const char *name() override;

      protected:
        explicit RenderingSystemDefault();
        ~RenderingSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;
    };

} // namespace CoronaEngine

#endif // CORONAENGINE_RENDERSYSTEMDEFAULT_HPP
