//
// Created by 47226 on 2025/9/4.
//

#ifndef CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#define CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
#include "Engine.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

#include <unordered_set>

namespace Corona
{

    class AnimationSystemDefault final : public BaseMultimediaSystem
    {
      public:
        static AnimationSystemDefault &inst();

        const char *name() override;

      protected:
        explicit AnimationSystemDefault();
        ~AnimationSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key
        std::queue<DataCache::id_type> unhandled_data_keys; // 当前帧未处理的key
    };

} // namespace Corona

#endif // CORONAENGINE_ANIMATIONSYSTEMDEFAULT_HPP
