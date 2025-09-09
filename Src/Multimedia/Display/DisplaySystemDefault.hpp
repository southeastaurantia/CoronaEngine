//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
#define CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
#include "Engine.h"
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

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key
        std::queue<DataCache::id_type> unhandled_data_keys; // 当前帧未处理的key

        void processDisplay(DataCache::id_type id);
    };

} // namespace Corona

#endif // CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
