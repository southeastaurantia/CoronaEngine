//
// Created by 47226 on 2025/9/5.
//

#ifndef CABBAGEFRAMEWORK_RENDERSYSTEMDEFAULT_HPP
#define CABBAGEFRAMEWORK_RENDERSYSTEMDEFAULT_HPP
#include "Multimedia/BaseRenderingSystem.hpp"

namespace CoronaEngine
{

    class RenderingSystemDefault final : public BaseRenderingSystem
    {
      public:
        static RenderingSystemDefault & get_singleton();

        const char *name() override;

      protected:
        explicit RenderingSystemDefault(FPS fps);
        ~RenderingSystemDefault() override;

        void _start() override;
        void _tick() override;
        void _stop() override;
    };

} // namespace CoronaEngine

#endif // CABBAGEFRAMEWORK_RENDERSYSTEMDEFAULT_HPP
