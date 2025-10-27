#include "test_registry.h"
#include "test_support.h"

#include <cassert>
#include <filesystem>

#include "corona/framework/plugin/plugin_manifest.h"

void plugin_manifest_file_example() {
    const test_scope test_marker("plugin_manifest_file_example");
    namespace fs = std::filesystem;
    auto asset_root = fs::path(CORONA_FRAMEWORK_TEST_ASSET_DIR);
    if (asset_root.empty()) {
        asset_root = fs::path(__FILE__).parent_path().parent_path() / "examples" / "corona_framework";
    }

    auto sample_path = fs::weakly_canonical(asset_root / "sample_manifest.json");
    std::cout << "[plugin_manifest_file_example][config] sample_path=" << sample_path << std::endl;
    auto manifest = corona::framework::plugin::load_manifest(sample_path);
    assert(manifest.name == "corona_framework_demo");
    assert(manifest.systems.size() == 2);
    assert(manifest.systems[1].dependencies.size() == 1);
}
