#include <cassert>
#include <string>

#include "corona/framework/plugin/plugin_manifest.h"
#include "test_registry.h"
#include "test_support.h"

void plugin_manifest_tests() {
    const test_scope test_marker("plugin_manifest_tests");
    const std::string json = R"JSON({
        "name": "example",
        "version": "1.0",
        "dependencies": ["core"],
        "systems": [
            {
                "id": "sample.system",
                "factory": "sample.factory",
                "dependencies": ["render"],
                "tags": ["demo"],
                "tick_ms": 8
            }
        ]
    })JSON";

    auto manifest = corona::framework::plugin::parse_manifest(json);
    assert(manifest.name == "example");
    assert(manifest.systems.size() == 1);
    assert(manifest.systems[0].factory == "sample.factory");
    assert(manifest.systems[0].tick_interval.count() == 8);
}
