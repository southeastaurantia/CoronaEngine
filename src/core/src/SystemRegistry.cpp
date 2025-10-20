#include <corona/core/SystemRegistry.h>

#include <algorithm>
#include <cstdint>

namespace Corona {

namespace {
    template <typename T>
    void append_unique(std::vector<T>& target, const T& value) {
        if (std::find(target.begin(), target.end(), value) == target.end()) {
            target.push_back(value);
        }
    }
}

bool SystemRegistry::register_plugin(SystemPluginDescriptor descriptor) {
    if (descriptor.name.empty() || !descriptor.factory) {
        return false;
    }
    return descriptors_.try_emplace(descriptor.name, std::move(descriptor)).second;
}

bool SystemRegistry::unregister_plugin(std::string_view name) {
    return descriptors_.erase(std::string{name}) > 0;
}

bool SystemRegistry::contains(std::string_view name) const {
    return descriptors_.find(std::string{name}) != descriptors_.end();
}

const SystemPluginDescriptor* SystemRegistry::find(std::string_view name) const {
    if (auto it = descriptors_.find(std::string{name}); it != descriptors_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const SystemPluginDescriptor*> SystemRegistry::list() const {
    std::vector<const SystemPluginDescriptor*> out;
    out.reserve(descriptors_.size());
    for (auto& [_, desc] : descriptors_) {
        out.push_back(&desc);
    }
    std::sort(out.begin(), out.end(), [](const SystemPluginDescriptor* lhs, const SystemPluginDescriptor* rhs) {
        return lhs->name < rhs->name;
    });
    return out;
}

SystemRegistry::Resolution SystemRegistry::resolve(const std::vector<std::string>& requested) const {
    return resolve_internal(requested);
}

SystemRegistry::Resolution SystemRegistry::resolve_internal(const std::vector<std::string>& requested) const {
    Resolution resolution;
    if (descriptors_.empty()) {
        return resolution;
    }

    enum class State : std::uint8_t { None, Visiting, Visited };

    std::unordered_map<std::string, State> visit_state;
    visit_state.reserve(descriptors_.size());
    std::vector<std::string> stack;
    stack.reserve(descriptors_.size());

    std::function<void(const std::string&)> dfs = [&](const std::string& name) {
        auto it = descriptors_.find(name);
        if (it == descriptors_.end()) {
            append_unique(resolution.missing, name);
            return;
        }

        State& state = visit_state[name];
        if (state == State::Visited) {
            return;
        }
        if (state == State::Visiting) {
            auto cycle_start = std::find(stack.begin(), stack.end(), name);
            std::vector<std::string> cycle;
            if (cycle_start != stack.end()) {
                cycle.assign(cycle_start, stack.end());
            }
            cycle.push_back(name);
            if (!cycle.empty()) {
                resolution.cycles.push_back(std::move(cycle));
            }
            return;
        }

        state = State::Visiting;
        stack.push_back(name);
        for (const auto& dep : it->second.dependencies) {
            dfs(dep);
        }
        stack.pop_back();
        state = State::Visited;
        resolution.order.push_back(&it->second);
    };

    std::vector<std::string> targets = requested;
    if (targets.empty()) {
        targets.reserve(descriptors_.size());
        for (const auto& [name, _] : descriptors_) {
            targets.push_back(name);
        }
        std::sort(targets.begin(), targets.end());
    }

    for (const auto& name : targets) {
        dfs(name);
    }

    std::reverse(resolution.order.begin(), resolution.order.end());
    return resolution;
}

std::vector<std::shared_ptr<ISystem>> SystemRegistry::instantiate(const Resolution& resolution,
                                                                  const Interfaces::SystemContext& context) const {
    std::vector<std::shared_ptr<ISystem>> instances;
    if (!resolution.success()) {
        return instances;
    }

    instances.reserve(resolution.order.size());
    for (const auto* descriptor : resolution.order) {
        if (!descriptor || !descriptor->factory) {
            continue;
        }
        auto instance = descriptor->factory(context);
        if (instance) {
            instances.emplace_back(std::move(instance));
        }
    }
    return instances;
}

} // namespace Corona
