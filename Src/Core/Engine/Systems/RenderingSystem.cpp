#include "RenderingSystem.h"

#include "Core/Engine/Engine.h"
#include "Core/Log.h"
#include "Resource/Mesh.h"
#include <memory>


using namespace Corona;

RenderingSystem::RenderingSystem()
    : ThreadedSystem("RenderingSystem")
{
    Engine::Instance().AddQueue(name(), std::make_unique<SafeCommandQueue>());
}

void RenderingSystem::onStart()
{
}

void RenderingSystem::onTick()
{
    // 最小消费命令队列，避免无限增长
    auto &rq = Engine::Instance().GetQueue(name());
    int spun = 0;
    while (spun < 16 && rq.try_execute())
        ++spun;

    // 遍历 data_keys_ 示例：从 Cache<Mesh> 读取并执行占位渲染操作
    auto &meshCache = Engine::Instance().Cache<Mesh>();
    meshCache.safe_loop_foreach(data_keys_, [&](std::shared_ptr<Mesh> m) {
        (void)m; // TODO: 真正的渲染逻辑
    });
}

void RenderingSystem::onStop()
{
}

void RenderingSystem::processRender(uint64_t /*id*/)
{
    updateEngine();
}

void RenderingSystem::updateEngine()
{
    // 按旧逻辑分阶段：GBuffer -> Composite
    gbufferPipeline();
    compositePipeline();
}
void RenderingSystem::gbufferPipeline()
{
    // TODO: 设置视图投影、写入 G-Buffer
    CE_LOG_DEBUG("RenderingSystem: gbufferPipeline executed");
}
void RenderingSystem::compositePipeline()
{
    // TODO: 合成阶段、灯光与最终输出
    CE_LOG_DEBUG("RenderingSystem: compositePipeline executed");
}

// static
void RenderingSystem::WatchMesh(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.insert(id);
    });
}

// static
void RenderingSystem::UnwatchMesh(uint64_t id)
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([id, &sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.erase(id);
    });
}

// static
void RenderingSystem::ClearWatched()
{
    auto &q = Engine::Instance().GetQueue("RenderingSystem");
    q.enqueue([&sys = Engine::Instance().GetSystem<RenderingSystem>()]() mutable {
        sys.data_keys_.clear();
    });
}
