#pragma once

#include "../IResourceLoader.h"
#include <string>
#include <vector>

namespace Corona
{
    struct BinaryResource : public IResource
    {
        std::vector<unsigned char> data;
    };

    class BinaryResourceLoader : public IResourceLoader
    {
      public:
        explicit BinaryResourceLoader(std::vector<std::string> exts = {".bin", ".dat"}) : exts_(std::move(exts))
        {
        }

        bool supports(const ResourceId &id) const override;
        std::shared_ptr<IResource> load(const ResourceId &id) override;

      private:
        std::vector<std::string> exts_;
    };
} // namespace Corona
