//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
#define CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
#include "Core/Engine.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace Corona
{

    class DisplaySystemDefault final : public BaseMultimediaSystem
    {
      public:
        const char *name() override;

      public:
        explicit DisplaySystemDefault();
        ~DisplaySystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key

        std::thread displayThread;
        std::atomic<bool> running;

        void processDisplay(DataCache::id_type id);
    };

} // namespace Corona

#endif // CORONAENGINE_DISPLAYSYSTEMDEFAULT_HPP
