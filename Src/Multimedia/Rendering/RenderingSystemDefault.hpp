//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_RENDERSYSTEMDEFAULT_HPP
#define CORONAENGINE_RENDERSYSTEMDEFAULT_HPP
#include "Engine.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace Corona
{

    class RenderingSystemDefault final : public BaseMultimediaSystem
    {
      public:
        static RenderingSystemDefault &inst();

        const char *name() override;

      protected:
        explicit RenderingSystemDefault();
        ~RenderingSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key
        std::queue<DataCache::id_type> unhandled_data_keys; // 当前帧未处理的key》
    };

} // namespace Corona

#endif // CORONAENGINE_RENDERSYSTEMDEFAULT_HPP
