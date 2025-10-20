#include <corona/core/detail/SystemHubs.h>

#include <iostream>
#include <memory>
#include <unordered_set>

using namespace Corona;

namespace {
struct SamplePayload {
    int counter = 0;
};

struct SampleEvent {
    int value = 0;
};
}  // namespace

int main() {
    DataCacheHub cacheHub;
    EventBusHub busHub;

    auto& cache = cacheHub.get<SamplePayload>();
    cache.insert(1, std::make_shared<SamplePayload>());

    cache.modify(1, [](std::shared_ptr<SamplePayload> payload) {
        if (payload) {
            payload->counter += 42;
        }
    });

    std::unordered_set<SafeDataCache<SamplePayload>::id_type> ids{1};
    cache.safe_loop_foreach(ids, [](SafeDataCache<SamplePayload>::id_type id, std::shared_ptr<SamplePayload> payload) {
        if (payload) {
            std::cout << "Payload " << id << " counter=" << payload->counter << "\n";
        }
    });

    const auto snapshot = cache.get(1);
    const int payloadValue = snapshot ? snapshot->counter : 0;

    auto& bus = busHub.get<SampleEvent>();
    auto subscription = bus.subscribe("system_hubs_sanity");
    bus.publish("system_hubs_sanity", SampleEvent{payloadValue});

    if (subscription.queue) {
        SampleEvent event;
        if (subscription.queue->try_pop(event)) {
            std::cout << "Received event value=" << event.value << "\n";
        }
    }

    bus.unsubscribe(subscription.topic, subscription.id);
    return 0;
}
