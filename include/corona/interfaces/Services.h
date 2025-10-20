#pragma once

#include <corona/interfaces/Concurrency.h>

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace Corona {
class IResource;
struct ResourceId;
} // namespace Corona

namespace Corona::Interfaces {

// 日志服务，用于桥接现有 corona_logger 输出。
class ILogger {
  public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

    virtual ~ILogger() = default;

    virtual void log(Level level, std::string_view message) = 0;

    void trace(std::string_view message) { log(Level::Trace, message); }
    void debug(std::string_view message) { log(Level::Debug, message); }
    void info(std::string_view message) { log(Level::Info, message); }
    void warn(std::string_view message) { log(Level::Warn, message); }
    void error(std::string_view message) { log(Level::Error, message); }
    void critical(std::string_view message) { log(Level::Critical, message); }
};

// 资源加载服务，抽象 ResourceManager 提供的关键能力。
class IResourceService {
  public:
    using ResourcePtr = std::shared_ptr<Corona::IResource>;
    using LoadCallback = std::function<void(const Corona::ResourceId&, ResourcePtr)>;

    virtual ~IResourceService() = default;

    virtual ResourcePtr load(const Corona::ResourceId& id) = 0;
    virtual ResourcePtr load_once(const Corona::ResourceId& id) = 0;
    virtual void load_async(const Corona::ResourceId& id, LoadCallback cb) = 0;
    virtual void load_once_async(const Corona::ResourceId& id, LoadCallback cb) = 0;
    virtual void preload(const std::vector<Corona::ResourceId>& ids) = 0;
    virtual void clear() = 0;
};

// 命令队列调度服务，用于统一创建与查询队列。
class ICommandScheduler {
  public:
    using QueueHandle = std::shared_ptr<ICommandQueue>;

    virtual ~ICommandScheduler() = default;

    virtual QueueHandle create_queue(std::string_view name) = 0;
    virtual QueueHandle get_queue(std::string_view name) = 0;
    virtual bool remove_queue(std::string_view name) = 0;
    virtual bool contains(std::string_view name) const = 0;
};

} // namespace Corona::Interfaces
