#pragma once
// Moved from Src/Core/IO/IResourceLoader.h
#include "IResource.h"
#include "ResourceTypes.h"
#include <memory>

namespace Corona
{
    struct IResourceLoader
    {
        virtual ~IResourceLoader() = default;
        virtual bool supports(const ResourceId &id) const = 0;
        virtual std::shared_ptr<IResource> load(const ResourceId &id) = 0;
    };
} // namespace Corona
