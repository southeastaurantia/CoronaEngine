#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace corona::framework::plugin {

struct plugin_manifest {
    struct system_entry {
        std::string id;
        std::string factory;
        std::vector<std::string> dependencies;
        std::vector<std::string> tags;
        std::chrono::milliseconds tick_interval{16};
    };

    std::string name;
    std::string version;
    std::vector<std::string> dependencies;
    std::vector<system_entry> systems;
};

plugin_manifest parse_manifest(std::string_view json_text);
plugin_manifest load_manifest(const std::filesystem::path& path);

}  // namespace corona::framework::plugin
