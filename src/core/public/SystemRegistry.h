#pragma once

#include <corona/interfaces/ISystem.h>
#include <corona/interfaces/SystemContext.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Corona {

struct SystemPluginDescriptor {
    using Factory = std::function<std::shared_ptr<ISystem>(const Interfaces::SystemContext&)>;

    std::string name;                                 // unique identifier for this system
    std::vector<std::string> dependencies;            // other system names this system requires
    Factory factory;                                  // factory that builds the system instance
    std::string description;                          // optional human readable summary
};

class SystemRegistry {
  public:
    struct Resolution {
        std::vector<const SystemPluginDescriptor*> order;
        std::vector<std::string> missing;
        std::vector<std::vector<std::string>> cycles;

        [[nodiscard]] bool success() const { return missing.empty() && cycles.empty(); }
    };

    bool register_plugin(SystemPluginDescriptor descriptor);
    bool unregister_plugin(std::string_view name);

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] const SystemPluginDescriptor* find(std::string_view name) const;

    [[nodiscard]] std::vector<const SystemPluginDescriptor*> list() const;

    [[nodiscard]] Resolution resolve(const std::vector<std::string>& requested) const;

    [[nodiscard]] std::vector<std::shared_ptr<ISystem>> instantiate(const Resolution& resolution,
                                                                    const Interfaces::SystemContext& context) const;

  private:
    Resolution resolve_internal(const std::vector<std::string>& requested) const;

    std::unordered_map<std::string, SystemPluginDescriptor> descriptors_;
};

} // namespace Corona
