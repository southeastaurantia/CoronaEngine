#pragma once

#include "../IResourceLoader.h"
#include <string>
#include <vector>

namespace Corona
{
    struct TextResource : public IResource
    {
        std::string text;
    };

    class TextResourceLoader : public IResourceLoader
    {
      public:
        explicit TextResourceLoader(std::vector<std::string> exts = {".txt", ".glsl", ".json"}) : exts_(std::move(exts))
        {
        }

        bool supports(const ResourceId &id) const override;
        std::shared_ptr<IResource> load(const ResourceId &id) override;

      private:
        std::vector<std::string> exts_;
    };
} // namespace Corona
