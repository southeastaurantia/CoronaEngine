//
// Created by 47226 on 2025/9/5.
//

#pragma once
#include "Core/Engine.h"
#include "Core/Thread/SafeCommandQueue.h"
#include "Multimedia/BaseMultimediaSystem.hpp"

namespace Corona
{

    class RenderingSystemDefault final : public BaseMultimediaSystem
    {
      public:
        const char *name() override;

      public:
        explicit RenderingSystemDefault();
        ~RenderingSystemDefault() override;

      public:
        void start() override;
        void tick() override;
        void stop() override;

      private:
        std::unordered_set<DataCache::id_type> data_keys;   // 全局DataCache的所有key

        std::thread renderThread;
        std::atomic<bool> running;

        void processRender(DataCache::id_type id);

        void updateEngine();
        void gbufferPipeline();
        void compositePipeline();
    };

} // namespace Corona

