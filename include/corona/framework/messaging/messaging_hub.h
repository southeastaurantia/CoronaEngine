#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "corona/framework/messaging/command_channel.h"
#include "corona/framework/messaging/data_projection.h"
#include "corona/framework/messaging/event_stream.h"

namespace corona::framework::messaging {

class messaging_hub {
   public:
    messaging_hub() = default;
    ~messaging_hub() = default;

    messaging_hub(const messaging_hub&) = delete;
    messaging_hub& operator=(const messaging_hub&) = delete;

    template <typename T>
    std::shared_ptr<event_stream<T>> acquire_event_stream(std::string_view topic);

    template <typename Request, typename Response>
    std::shared_ptr<command_channel<Request, Response>> acquire_command_channel(std::string_view topic);

    template <typename T>
    std::shared_ptr<data_projection<T>> acquire_projection(std::string_view topic);

   private:
    using storage_map = std::unordered_map<std::string, std::shared_ptr<void>>;

    std::mutex mutex_;
    storage_map event_streams_;
    storage_map command_channels_;
    storage_map projections_;
};

template <typename T>
std::shared_ptr<event_stream<T>> messaging_hub::acquire_event_stream(std::string_view topic) {
    auto key = std::string(topic);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = event_streams_.find(key);
    if (it == event_streams_.end()) {
        auto stream = std::make_shared<event_stream<T>>();
        event_streams_.emplace(key, stream);
        return stream;
    }
    auto stream = std::static_pointer_cast<event_stream<T>>(it->second);
    if (!stream) {
        throw std::runtime_error("event stream type mismatch");
    }
    return stream;
}

template <typename Request, typename Response>
std::shared_ptr<command_channel<Request, Response>> messaging_hub::acquire_command_channel(std::string_view topic) {
    auto key = std::string(topic);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = command_channels_.find(key);
    if (it == command_channels_.end()) {
        auto channel = std::make_shared<command_channel<Request, Response>>();
        command_channels_.emplace(key, channel);
        return channel;
    }
    auto channel = std::static_pointer_cast<command_channel<Request, Response>>(it->second);
    if (!channel) {
        throw std::runtime_error("command channel type mismatch");
    }
    return channel;
}

template <typename T>
std::shared_ptr<data_projection<T>> messaging_hub::acquire_projection(std::string_view topic) {
    auto key = std::string(topic);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projections_.find(key);
    if (it == projections_.end()) {
        auto projection = std::make_shared<data_projection<T>>();
        projections_.emplace(key, projection);
        return projection;
    }
    auto projection = std::static_pointer_cast<data_projection<T>>(it->second);
    if (!projection) {
        throw std::runtime_error("projection type mismatch");
    }
    return projection;
}

}  // namespace corona::framework::messaging
