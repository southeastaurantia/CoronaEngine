#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace Corona::Interfaces {

// 命令队列抽象：系统线程通过它收发跨线程任务。
class ICommandQueue {
   public:
    virtual ~ICommandQueue() = default;

    virtual void enqueue(std::function<void()> command) = 0;
    virtual bool try_execute() = 0;
    virtual bool empty() const = 0;
};

// 数据缓存抽象：按 id 管理共享状态。
class IDataCache {
   public:
    using id_type = std::uint64_t;

    virtual ~IDataCache() = default;

    virtual bool insert(id_type id, std::shared_ptr<void> payload) = 0;
    virtual bool erase(id_type id) = 0;
    virtual std::shared_ptr<const void> get(id_type id) const = 0;
};

// 事件通道抽象：发布订阅消息。
class IEventChannel {
   public:
    virtual ~IEventChannel() = default;
};

}  // namespace Corona::Interfaces
