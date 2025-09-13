#pragma once

#include <memory>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace Corona
{
    using ResourceType = std::string; // 资源类型标识（如"texture","mesh"），可自定义
    using ResourcePath = std::string; // 资源路径或URI

    struct ResourceId
    {
        ResourceType type;
        ResourcePath path;
        // 全局唯一 ID：由规范化后的 (type + "\n" + path) 通过 FNV1a-64 计算
        // 说明：保持与旧代码兼容，未通过工厂创建时 uid 可能为 0；哈希会在需要时按 (type,path) 计算。
        std::uint64_t uid{0};

        // 工具：计算 uid（纯函数，不修改当前对象）
        static std::uint64_t ComputeUid(const ResourceType &type, const ResourcePath &path)
        {
            auto normType = type; // type 也统一为小写，避免大小写差异
            std::transform(normType.begin(), normType.end(), normType.begin(), [](unsigned char c) { return std::tolower(c); });

            auto normPath = path;
            // 路径规范化：\\ → /，去重 //，去尾部 /，统一小写（跨平台一致性）
            std::replace(normPath.begin(), normPath.end(), '\\', '/');
            // 去重连续 '/'
            normPath.erase(std::unique(normPath.begin(), normPath.end(), [](char a, char b) { return a == '/' && b == '/'; }), normPath.end());
            // 去尾部 '/'
            if (!normPath.empty() && normPath.back() == '/') normPath.pop_back();
            std::transform(normPath.begin(), normPath.end(), normPath.begin(), [](unsigned char c) { return std::tolower(c); });

            // FNV-1a 64 位
            const std::uint64_t FNV_OFFSET = 1469598103934665603ull;
            const std::uint64_t FNV_PRIME = 1099511628211ull;
            auto fnv1a64 = [&](const std::string &s, std::uint64_t seed) {
                std::uint64_t h = seed;
                for (unsigned char ch : s) {
                    h ^= static_cast<std::uint64_t>(ch);
                    h *= FNV_PRIME;
                }
                return h;
            };

            std::uint64_t h = FNV_OFFSET;
            h = fnv1a64(normType, h);
            h = fnv1a64(std::string("\n"), h);
            h = fnv1a64(normPath, h);
            return h;
        }

        // 工厂：按规范化规则生成带 uid 的 ResourceId（推荐）
        static ResourceId From(ResourceType type, ResourcePath path)
        {
            ResourceId id;
            id.type = std::move(type);
            id.path = std::move(path);
            id.uid = ComputeUid(id.type, id.path);
            return id;
        }

        bool operator==(const ResourceId &o) const noexcept
        {
            const std::uint64_t a = uid ? uid : ComputeUid(type, path);
            const std::uint64_t b = o.uid ? o.uid : ComputeUid(o.type, o.path);
            return a == b;
        }
        bool operator<(const ResourceId &o) const noexcept
        {
            const std::uint64_t a = uid ? uid : ComputeUid(type, path);
            const std::uint64_t b = o.uid ? o.uid : ComputeUid(o.type, o.path);
            return a < b;
        }
    };

    struct ResourceIdHash
    {
        std::size_t operator()(const ResourceId &id) const noexcept
        {
            // 优先使用 uid；若 uid==0（向后兼容），按 (type,path) 规范化计算 uid 再返回
            std::uint64_t u = id.uid ? id.uid : ResourceId::ComputeUid(id.type, id.path);
            return static_cast<std::size_t>(u);
        }
    };

    template <typename T>
    using ResourcePtr = std::shared_ptr<T>;

    // 通用子资源标识（与具体解析库无关）
    enum class SubResourceKind : std::uint32_t
    {
        Unknown = 0,
        Geometry = 1,      // 几何体定义（子网格、mesh、primitive）
        GeometryInstance,  // 几何体实例（某节点/实体引用）
        Material,
        Texture,
        Node,
        Animation,
        Other = 255
    };

    struct SubResourceId
    {
        std::uint64_t uid{0};         // 全局唯一子资源 ID
        std::uint64_t parentUid{0};   // 父资源（通常为容器资源，如模型/包）的 uid
        SubResourceKind kind{SubResourceKind::Unknown};

        // 构造：用父 uid + kind + 本地数值键（如 index）+ 可选字符串键（如路径/名字）组合
        static std::uint64_t Compute(std::uint64_t parent,
                                     SubResourceKind kind,
                                     std::uint64_t localNumeric,
                                     std::string_view localString = {})
        {
            const std::uint64_t FNV_OFFSET = 1469598103934665603ull;
            const std::uint64_t FNV_PRIME  = 1099511628211ull;
            auto h = FNV_OFFSET;
            auto mix64 = [&](std::uint64_t v){
                for (int i=0;i<8;++i){ unsigned char b = static_cast<unsigned char>(v & 0xFF); h ^= b; h *= FNV_PRIME; v >>= 8; }
            };
            mix64(parent);
            mix64(static_cast<std::uint64_t>(kind));
            mix64(localNumeric);
            for (unsigned char c : localString) { h ^= c; h *= FNV_PRIME; }
            return h;
        }

        static SubResourceId FromIndex(const ResourceId &parent, SubResourceKind kind, std::uint64_t index)
        {
            SubResourceId s;
            s.parentUid = parent.uid ? parent.uid : ResourceId::ComputeUid(parent.type, parent.path);
            s.kind = kind;
            s.uid = Compute(s.parentUid, kind, index);
            return s;
        }

        static SubResourceId FromKey(const ResourceId &parent, SubResourceKind kind, std::string_view key)
        {
            SubResourceId s;
            s.parentUid = parent.uid ? parent.uid : ResourceId::ComputeUid(parent.type, parent.path);
            s.kind = kind;
            s.uid = Compute(s.parentUid, kind, 0, key);
            return s;
        }

        static SubResourceId Compose(std::uint64_t parentUid, SubResourceKind kind,
                                     std::uint64_t localNumeric, std::string_view localString = {})
        {
            SubResourceId s;
            s.parentUid = parentUid;
            s.kind = kind;
            s.uid = Compute(parentUid, kind, localNumeric, localString);
            return s;
        }
    };
} // namespace Corona
