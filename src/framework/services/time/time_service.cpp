#include "corona/framework/services/time/time_service.h"

#include <mutex>

#include "corona/framework/service/service_provider.h"

namespace corona::framework::services::time {

namespace {

class steady_time_service final : public time_service {
   public:
    steady_time_service()
        : start_time_(steady_clock::now()),
          current_time_(start_time_),
          last_delta_(steady_clock::duration::zero()),
          elapsed_(steady_clock::duration::zero()),
          frame_index_(0) {}

    ~steady_time_service() override = default;

    steady_clock::time_point start_time() const override {
        std::scoped_lock lock(mutex_);
        return start_time_;
    }

    steady_clock::time_point current_time() const override {
        std::scoped_lock lock(mutex_);
        return current_time_;
    }

    steady_clock::duration time_since_start() const override {
        std::scoped_lock lock(mutex_);
        return elapsed_;
    }

    steady_clock::duration last_frame_duration() const override {
        std::scoped_lock lock(mutex_);
        return last_delta_;
    }

    std::uint64_t frame_index() const override {
        std::scoped_lock lock(mutex_);
        return frame_index_;
    }

    time_snapshot snapshot() const override {
        std::scoped_lock lock(mutex_);
        return time_snapshot{
            .start_time = start_time_,
            .current_time = current_time_,
            .delta_time = last_delta_,
            .elapsed_time = elapsed_,
            .frame_index = frame_index_,
        };
    }

    void advance_frame() override {
        update_with_timestamp(steady_clock::time_point{});
    }

    void advance_frame(steady_clock::time_point timestamp) override {
        update_with_timestamp(timestamp);
    }

   private:
    void update_with_timestamp(steady_clock::time_point timestamp) {
        auto now = timestamp;
        if (now == steady_clock::time_point{}) {
            now = steady_clock::now();
        }

        std::scoped_lock lock(mutex_);
        if (now < current_time_) {
            now = current_time_;
        }

        steady_clock::duration delta = now - current_time_;
        if (frame_index_ == 0) {
            delta = now - start_time_;
        }

        last_delta_ = delta;
        current_time_ = now;
        elapsed_ = current_time_ - start_time_;
        ++frame_index_;
    }

    mutable std::mutex mutex_;
    steady_clock::time_point start_time_;
    steady_clock::time_point current_time_;
    steady_clock::duration last_delta_;
    steady_clock::duration elapsed_;
    std::uint64_t frame_index_;
};

}  // namespace

time_service_ptr make_time_service() {
    return std::make_shared<steady_time_service>();
}

time_service_ptr register_time_service(service::service_collection& collection) {
    if (collection.contains<time_service>()) {
        auto provider = collection.build_service_provider();
        if (auto resolved = provider.get_service<time_service>()) {
            return resolved;
        }
    }
    auto instance = make_time_service();
    collection.add_singleton<time_service>(instance);
    return instance;
}

}  // namespace corona::framework::services::time
