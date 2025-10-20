#include "ResourceServiceAdapter.h"

#include <utility>

namespace Corona::Core {

ResourceServiceAdapter::ResourceServiceAdapter(std::shared_ptr<ResourceManager> manager)
    : manager_(std::move(manager)) {}

ResourceServiceAdapter::ResourcePtr ResourceServiceAdapter::load(const Corona::ResourceId& id) {
    return manager_ ? manager_->load(id) : nullptr;
}

ResourceServiceAdapter::ResourcePtr ResourceServiceAdapter::load_once(const Corona::ResourceId& id) {
    return manager_ ? manager_->load_once(id) : nullptr;
}

void ResourceServiceAdapter::load_async(const Corona::ResourceId& id, LoadCallback cb) {
    if (!manager_) {
        return;
    }
    manager_->load_async(id, std::move(cb));
}

void ResourceServiceAdapter::load_once_async(const Corona::ResourceId& id, LoadCallback cb) {
    if (!manager_) {
        return;
    }
    manager_->load_once_async(id, std::move(cb));
}

void ResourceServiceAdapter::preload(const std::vector<Corona::ResourceId>& ids) {
    if (!manager_) {
        return;
    }
    manager_->preload(ids);
}

void ResourceServiceAdapter::clear() {
    if (!manager_) {
        return;
    }
    manager_->clear();
}

void ResourceServiceAdapter::set_manager(std::shared_ptr<ResourceManager> manager) {
    manager_ = std::move(manager);
}

}  // namespace Corona::Core
