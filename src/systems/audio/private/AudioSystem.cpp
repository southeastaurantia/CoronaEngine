#include "AudioSystem.h"
#include "Engine.h"
#include <filesystem>

using namespace Corona;

AudioSystem::AudioSystem()
    : ThreadedSystem("AudioSystem")
{
    Engine::instance().add_queue(name(), std::make_unique<SafeCommandQueue>());
}

void AudioSystem::onStart()
{
    // 测试样例：系统内部使用资源管理器异步加载 shader，并把结果回投到本系统队列
    const auto assets_root = (std::filesystem::current_path() / "assets").string();
    auto shaderId = ResourceId::from("shader", assets_root);
    auto sys_name = std::string{name()};
    Engine::instance().resources().load_once_async(
        shaderId,
        [sys_name](const ResourceId&, std::shared_ptr<IResource> r) {
            auto &engine = Engine::instance();
            auto &q = engine.get_queue(sys_name);
            if (!r) {
                q.enqueue([] { CE_LOG_WARN("[AudioSystem] 异步加载 shader 失败"); });
                return;
            }
            q.enqueue([] { CE_LOG_INFO("[AudioSystem] 异步加载 shader 成功（测试样例）"); });
        });
}
void AudioSystem::onTick()
{
    auto &rq = Engine::instance().get_queue(name());
    int spun = 0;
    while (spun < 100 && !rq.empty())
    {
        if (!rq.try_execute())
            continue;
        ++spun;
    }
}
void AudioSystem::onStop()
{
}
void AudioSystem::process_audio(uint64_t /*id*/)
{
}
