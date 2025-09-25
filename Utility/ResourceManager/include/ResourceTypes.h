#pragma once
// Moved from Src/Core/IO/ResourceTypes.h (same content, comments trimmed)
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
namespace Corona {
    using ResourceType = std::string; using ResourcePath = std::string;
    struct ResourceId {
        ResourceType type; ResourcePath path; std::uint64_t uid{0};
        static std::uint64_t ComputeUid(const ResourceType &type, const ResourcePath &path);
        static ResourceId From(ResourceType type, ResourcePath path);
        bool operator==(const ResourceId &o) const noexcept; bool operator<(const ResourceId &o) const noexcept; };
    struct ResourceIdHash { std::size_t operator()(const ResourceId &id) const noexcept; };
    template <typename T> using ResourcePtr = std::shared_ptr<T>;
    enum class SubResourceKind : std::uint32_t { Unknown=0, Geometry=1, GeometryInstance, Material, Texture, Node, Animation, Other=255 };
    struct SubResourceId { std::uint64_t uid{0}; std::uint64_t parentUid{0}; SubResourceKind kind{SubResourceKind::Unknown};
        static std::uint64_t Compute(std::uint64_t parent, SubResourceKind kind, std::uint64_t localNumeric, std::string_view localString={} );
        static SubResourceId FromIndex(const ResourceId &parent, SubResourceKind kind, std::uint64_t index);
        static SubResourceId FromKey(const ResourceId &parent, SubResourceKind kind, std::string_view key);
        static SubResourceId Compose(std::uint64_t parentUid, SubResourceKind kind, std::uint64_t localNumeric, std::string_view localString={}); };
} // namespace Corona
