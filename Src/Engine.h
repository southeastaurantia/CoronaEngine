//
// Created by 47226 on 2025/9/8.
//

#ifndef CORONAENGINE_ENGINE_H
#define CORONAENGINE_ENGINE_H
#include <Core/Logger.h>

namespace Corona
{

    class Engine final
    {
      public:
        static Engine& inst();

        void init();

      private:
        Engine();
        ~Engine();
        Engine(const Engine &other) = delete;
        Engine &operator=(const Engine &other) = delete;

    };

} // namespace Corona

#endif // CORONAENGINE_ENGINE_H
