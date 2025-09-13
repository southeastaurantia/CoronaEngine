#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Corona
{
    using ResourceType = std::string; // 资源类型标识（如 "texture"、"mesh"），由调用方定义
    using ResourcePath = std::string; // 资源路径或 URI

    struct ResourceId
    {
        ResourceType type;
        ResourcePath path;
        std::uint64_t uid{0};

        // 计算规范化 uid（纯函数）：小写化 type/path，归一化分隔符并去重
        static std::uint64_t ComputeUid(const ResourceType &type, const ResourcePath &path);
        // 按规范化规则创建带 uid 的 ResourceId
        static ResourceId From(ResourceType type, ResourcePath path);

        bool operator==(const ResourceId &o) const noexcept;
        bool operator<(const ResourceId &o) const noexcept;
    };

    struct ResourceIdHash
    {
        std::size_t operator()(const ResourceId &id) const noexcept;
    };

    template <typename T>
    using ResourcePtr = std::shared_ptr<T>;

    // 通用子资源标识（与具体解析库无关），可唯一标识父资源内部的某个元素
    enum class SubResourceKind : std::uint32_t
    {
        Unknown = 0,
        Geometry = 1,     // 几何体定义（子网格、mesh、primitive）
        GeometryInstance, // 几何体实例（某节点/实体引用）
        Material,
        Texture,
        Node,
        Animation,
        Other = 255
    };

    struct SubResourceId
    {
        std::uint64_t uid{0};
        std::uint64_t parentUid{0};
        SubResourceKind kind{SubResourceKind::Unknown};

        static std::uint64_t Compute(std::uint64_t parent,
                                     SubResourceKind kind,
                                     std::uint64_t localNumeric,
                                     std::string_view localString = {});

        static SubResourceId FromIndex(const ResourceId &parent, SubResourceKind kind, std::uint64_t index);
        static SubResourceId FromKey(const ResourceId &parent, SubResourceKind kind, std::string_view key);
        static SubResourceId Compose(std::uint64_t parentUid, SubResourceKind kind,
                                     std::uint64_t localNumeric, std::string_view localString = {});
    };
} // namespace Corona
