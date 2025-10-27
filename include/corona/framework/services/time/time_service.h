#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include "corona/framework/service/service_collection.h"

namespace corona::framework::services::time {

using steady_clock = std::chrono::steady_clock;

struct time_snapshot {
    steady_clock::time_point start_time{};
    steady_clock::time_point current_time{};
    steady_clock::duration delta_time{};
    steady_clock::duration elapsed_time{};
    std::uint64_t frame_index = 0;
};

class time_service {
   public:
    virtual ~time_service() = default;

    virtual steady_clock::time_point start_time() const = 0;
    virtual steady_clock::time_point current_time() const = 0;
    virtual steady_clock::duration time_since_start() const = 0;
    virtual steady_clock::duration last_frame_duration() const = 0;
    virtual std::uint64_t frame_index() const = 0;
    virtual time_snapshot snapshot() const = 0;

    virtual void advance_frame() = 0;
    virtual void advance_frame(steady_clock::time_point timestamp) = 0;
};

using time_service_ptr = std::shared_ptr<time_service>;

[[nodiscard]] time_service_ptr register_time_service(service::service_collection& collection);

}  // namespace corona::framework::services::time
