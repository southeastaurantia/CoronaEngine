#include <corona/systems/DisplaySystem.h>

#include <filesystem>

#include <ResourceTypes.h>

using namespace Corona;

DisplaySystem::DisplaySystem()
    : ThreadedSystem("DisplaySystem")
{
}

void DisplaySystem::configure(const Interfaces::SystemContext &context)
{
    ThreadedSystem::configure(context);
    resource_service_ = services().try_get<Interfaces::IResourceService>();
    scheduler_ = services().try_get<Interfaces::ICommandScheduler>();
    if (scheduler_)
    {
        system_queue_handle_ = scheduler_->get_queue(name());
        if (!system_queue_handle_)
        {
            system_queue_handle_ = scheduler_->create_queue(name());
        }
    }
}

void DisplaySystem::onStart()
{
    if (!resource_service_)
    {
        CE_LOG_WARN("[DisplaySystem] 资源服务未注册，跳过示例加载");
        return;
    }
    if (!system_queue_handle_)
    {
        CE_LOG_WARN("[DisplaySystem] 命令队列句柄缺失，跳过示例加载");
        return;
    }

    const auto assets_root = (std::filesystem::current_path() / "assets").string();
    auto shaderId = ResourceId::from("shader", assets_root);
    auto queue_handle = system_queue_handle_;
    resource_service_->load_once_async(
        shaderId,
        [queue_handle](const ResourceId &, std::shared_ptr<IResource> r) {
            if (!queue_handle)
            {
                return;
            }
            queue_handle->enqueue([success = static_cast<bool>(r)] {
                if (!success)
                {
                    CE_LOG_WARN("[DisplaySystem] 异步加载 shader 失败");
                    return;
                }
                CE_LOG_INFO("[DisplaySystem] 异步加载 shader 成功（测试样例）");
            });
        });
}
void DisplaySystem::onTick()
{
    if (auto *queue = command_queue())
    {
        int spun = 0;
        while (spun < 100 && !queue->empty())
        {
            if (!queue->try_execute())
            {
                continue;
            }
            ++spun;
        }
    }
}
void DisplaySystem::onStop()
{
}
void DisplaySystem::process_display(uint64_t /*id*/)
{
}
