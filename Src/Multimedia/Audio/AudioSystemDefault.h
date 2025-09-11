//
// Created by 47226 on 2025/9/5.
//

#ifndef CORONAENGINE_AUDIOSYSTEMDEFAULT_HPP
#define CORONAENGINE_AUDIOSYSTEMDEFAULT_HPP
#include "Core/Engine.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace Corona
{

    class AudioSystemDefault final : public BaseMultimediaSystem
    {
      public:

        const char *name() override;

      public:
        explicit AudioSystemDefault();
        ~AudioSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key

        std::thread audioThread;
        std::atomic<bool> running;

        void processAudio(DataCache::id_type id);
    };

} // namespace Corona

#endif // CORONAENGINE_AUDIOSYSTEMDEFAULT_HPP
