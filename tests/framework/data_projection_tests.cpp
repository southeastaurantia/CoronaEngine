#include "test_registry.h"
#include "test_support.h"

#include <cassert>
#include <vector>

#include "corona/framework/messaging/data_projection.h"

void data_projection_tests() {
    const test_scope test_marker("data_projection_tests");
    corona::framework::messaging::data_projection<std::vector<int>> projection;
    std::vector<std::vector<int>> observed;

    auto subscription = projection.subscribe([&](const std::vector<int>& data) {
        observed.push_back(data);
    });

    projection.set(std::vector<int>{1, 2, 3});
    projection.set(std::vector<int>{4, 5});

    auto snapshot = projection.get_snapshot();
    assert(snapshot.size() == 2);
    assert(snapshot[0] == 4);

    assert(observed.size() == 2);
    assert(observed[0].size() == 3);
    assert(observed[1].size() == 2);

    subscription.release();
}
