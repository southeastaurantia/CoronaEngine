#pragma once

#include <memory>
#include <string>

namespace Corona
{
    using ResourceType = std::string; // 资源类型标识（如"texture","mesh"），可自定义
    using ResourcePath = std::string; // 资源路径或URI

    struct ResourceId
    {
        ResourceType type;
        ResourcePath path;

        bool operator==(const ResourceId &o) const noexcept
        {
            return type == o.type && path == o.path;
        }
        bool operator<(const ResourceId &o) const noexcept
        {
            return type < o.type || (type == o.type && path < o.path);
        }
    };

    struct ResourceIdHash
    {
        std::size_t operator()(const ResourceId &id) const noexcept
        {
            // 简单组合哈希
            std::hash<std::string> h;
            return (h(id.type) * 1315423911u) ^ h(id.path);
        }
    };

    template <typename T>
    using ResourcePtr = std::shared_ptr<T>;
} // namespace Corona
