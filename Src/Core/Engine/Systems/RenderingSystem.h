#pragma once

#include "Core/Engine/ThreadedSystem.h"
#include <unordered_set>


namespace Corona
{
    class RenderingSystem final : public ThreadedSystem
    {
      public:
        RenderingSystem();

        // 提供静态便捷方法，向渲染系统队列投递数据关注/取消命令
        static void WatchMesh(uint64_t id);   // 关注某个 Mesh 数据 id
        static void UnwatchMesh(uint64_t id); // 取消关注
        static void ClearWatched();           // 清空关注集合

      protected:
        void onStart() override;
        void onTick() override;
        void onStop() override;

      private:
        // 迁移保留：全局DataCache的所有key集合（后续用于 foreach）
        std::unordered_set<uint64_t> data_keys_{};

        // 对应旧版的渲染流程拆分
        void processRender(uint64_t id);
        void updateEngine();
        void gbufferPipeline();
        void compositePipeline();
    };
} // namespace Corona
