#include "test_registry.h"
#include "test_support.h"

#include <cassert>
#include <exception>

#include "corona/framework/plugin/plugin_manifest.h"

void plugin_manifest_failure_tests() {
    const test_scope test_marker("plugin_manifest_failure_tests");
    bool threw_missing_id = false;
    try {
        corona::framework::plugin::parse_manifest(R"({"systems":[{"factory":"missing"}]})");
    } catch (const std::exception&) {
        threw_missing_id = true;
    }
    assert(threw_missing_id);
}
