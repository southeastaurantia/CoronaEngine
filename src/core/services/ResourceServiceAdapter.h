#pragma once

#include <ResourceManager.h>
#include <corona/interfaces/Services.h>

#include <memory>

namespace Corona::Core {

class ResourceServiceAdapter final : public Interfaces::IResourceService {
   public:
    explicit ResourceServiceAdapter(std::shared_ptr<ResourceManager> manager);
    ~ResourceServiceAdapter() override = default;

    ResourcePtr load(const Corona::ResourceId& id) override;
    ResourcePtr load_once(const Corona::ResourceId& id) override;
    void load_async(const Corona::ResourceId& id, LoadCallback cb) override;
    void load_once_async(const Corona::ResourceId& id, LoadCallback cb) override;
    void preload(const std::vector<Corona::ResourceId>& ids) override;
    void clear() override;

    void set_manager(std::shared_ptr<ResourceManager> manager);

   private:
    std::shared_ptr<ResourceManager> manager_;
};

}  // namespace Corona::Core
