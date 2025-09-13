#pragma once

#include "IResource.h"
#include "ResourceTypes.h"
#include <memory>

namespace Corona
{
    // 可插拔资源加载器接口：按类型/扩展名/路径匹配，返回 IResource 智能指针
    struct IResourceLoader
    {
        virtual ~IResourceLoader() = default;

        // 是否支持该资源（可通过类型/扩展/URI scheme 判断）
        virtual bool supports(const ResourceId &id) const = 0;

        // 同步加载
        virtual std::shared_ptr<IResource> load(const ResourceId &id) = 0;
    };
} // namespace Corona
