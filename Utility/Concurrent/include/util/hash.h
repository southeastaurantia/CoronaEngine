#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace Corona::Concurrent::Util {

/**
 * 高性能哈希函数集合
 * 
 * 包含多种优化的哈希算法，适用于不同场景：
 * - xxHash：通用高性能哈希，适合大多数用途
 * - CityHash：Google 开发，针对字符串优化
 * - FNV：简单快速，适合小数据
 * - MurmurHash：分布均匀，适合哈希表
 */

namespace Hash {

// xxHash 32位高性能实现
namespace xxHash { // xxHash
    constexpr std::uint32_t PRIME32_1 = 0x9E3779B1U;
    constexpr std::uint32_t PRIME32_2 = 0x85EBCA77U;
    constexpr std::uint32_t PRIME32_3 = 0xC2B2AE3DU;
    constexpr std::uint32_t PRIME32_4 = 0x27D4EB2FU;
    constexpr std::uint32_t PRIME32_5 = 0x165667B1U;

    inline std::uint32_t rotl32(std::uint32_t x, int r) {
        return (x << r) | (x >> (32 - r));
    }

    inline std::uint32_t hash32(const void* data, std::size_t len, std::uint32_t seed = 0) {
        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        const std::uint8_t* end = p + len;
        std::uint32_t h32;

        if (len >= 16) {
            const std::uint8_t* limit = end - 16;
            std::uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
            std::uint32_t v2 = seed + PRIME32_2;
            std::uint32_t v3 = seed + 0;
            std::uint32_t v4 = seed - PRIME32_1;

            do {
                v1 = rotl32(v1 + (*(std::uint32_t*)p) * PRIME32_2, 13) * PRIME32_1;
                p += 4;
                v2 = rotl32(v2 + (*(std::uint32_t*)p) * PRIME32_2, 13) * PRIME32_1;
                p += 4;
                v3 = rotl32(v3 + (*(std::uint32_t*)p) * PRIME32_2, 13) * PRIME32_1;
                p += 4;
                v4 = rotl32(v4 + (*(std::uint32_t*)p) * PRIME32_2, 13) * PRIME32_1;
                p += 4;
            } while (p <= limit);

            h32 = rotl32(v1, 1) + rotl32(v2, 7) + rotl32(v3, 12) + rotl32(v4, 18);
        } else {
            h32 = seed + PRIME32_5;
        }

        h32 += static_cast<std::uint32_t>(len);

        while (p + 4 <= end) {
            h32 += (*(std::uint32_t*)p) * PRIME32_3;
            h32 = rotl32(h32, 17) * PRIME32_4;
            p += 4;
        }

        while (p < end) {
            h32 += (*p) * PRIME32_5;
            h32 = rotl32(h32, 11) * PRIME32_1;
            p++;
        }

        h32 ^= h32 >> 15;
        h32 *= PRIME32_2;
        h32 ^= h32 >> 13;
        h32 *= PRIME32_3;
        h32 ^= h32 >> 16;

        return h32;
    }

    // xxHash 64位高性能实现
    constexpr std::uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    constexpr std::uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    constexpr std::uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    constexpr std::uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    constexpr std::uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    inline std::uint64_t rotl64(std::uint64_t x, int r) {
        return (x << r) | (x >> (64 - r));
    }

    inline std::uint64_t hash64(const void* data, std::size_t len, std::uint64_t seed = 0) {
        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        const std::uint8_t* end = p + len;
        std::uint64_t h64;

        if (len >= 32) {
            const std::uint8_t* limit = end - 32;
            std::uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
            std::uint64_t v2 = seed + PRIME64_2;
            std::uint64_t v3 = seed + 0;
            std::uint64_t v4 = seed - PRIME64_1;

            do {
                v1 = rotl64(v1 + (*(std::uint64_t*)p) * PRIME64_2, 31) * PRIME64_1;
                p += 8;
                v2 = rotl64(v2 + (*(std::uint64_t*)p) * PRIME64_2, 31) * PRIME64_1;
                p += 8;
                v3 = rotl64(v3 + (*(std::uint64_t*)p) * PRIME64_2, 31) * PRIME64_1;
                p += 8;
                v4 = rotl64(v4 + (*(std::uint64_t*)p) * PRIME64_2, 31) * PRIME64_1;
                p += 8;
            } while (p <= limit);

            h64 = rotl64(v1, 1) + rotl64(v2, 7) + rotl64(v3, 12) + rotl64(v4, 18);

            v1 *= PRIME64_2; v1 = rotl64(v1, 31); v1 *= PRIME64_1; h64 ^= v1;
            h64 = h64 * PRIME64_1 + PRIME64_4;

            v2 *= PRIME64_2; v2 = rotl64(v2, 31); v2 *= PRIME64_1; h64 ^= v2;
            h64 = h64 * PRIME64_1 + PRIME64_4;

            v3 *= PRIME64_2; v3 = rotl64(v3, 31); v3 *= PRIME64_1; h64 ^= v3;
            h64 = h64 * PRIME64_1 + PRIME64_4;

            v4 *= PRIME64_2; v4 = rotl64(v4, 31); v4 *= PRIME64_1; h64 ^= v4;
            h64 = h64 * PRIME64_1 + PRIME64_4;
        } else {
            h64 = seed + PRIME64_5;
        }

        h64 += static_cast<std::uint64_t>(len);

        while (p + 8 <= end) {
            std::uint64_t k1 = (*(std::uint64_t*)p);
            k1 *= PRIME64_2; k1 = rotl64(k1, 31); k1 *= PRIME64_1;
            h64 ^= k1;
            h64 = rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
            p += 8;
        }

        if (p + 4 <= end) {
            h64 ^= static_cast<std::uint64_t>(*(std::uint32_t*)p) * PRIME64_1;
            h64 = rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
            p += 4;
        }

        while (p < end) {
            h64 ^= (*p) * PRIME64_5;
            h64 = rotl64(h64, 11) * PRIME64_1;
            p++;
        }

        h64 ^= h64 >> 33;
        h64 *= PRIME64_2;
        h64 ^= h64 >> 29;
        h64 *= PRIME64_3;
        h64 ^= h64 >> 32;

        return h64;
    }
}

// FNV 哈希（Fowler-Noll-Vo）
namespace FNV {
    constexpr std::uint32_t FNV_32_PRIME = 0x01000193;
    constexpr std::uint32_t FNV_32_OFFSET = 0x811C9DC5;
    constexpr std::uint64_t FNV_64_PRIME = 0x00000100000001B3ULL;
    constexpr std::uint64_t FNV_64_OFFSET = 0xCBF29CE484222325ULL;

    inline std::uint32_t hash32(const void* data, std::size_t len) {
        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        std::uint32_t hash = FNV_32_OFFSET;
        
        for (std::size_t i = 0; i < len; ++i) {
            hash ^= p[i];
            hash *= FNV_32_PRIME;
        }
        
        return hash;
    }

    inline std::uint64_t hash64(const void* data, std::size_t len) {
        const std::uint8_t* p = static_cast<const std::uint8_t*>(data);
        std::uint64_t hash = FNV_64_OFFSET;
        
        for (std::size_t i = 0; i < len; ++i) {
            hash ^= p[i];
            hash *= FNV_64_PRIME;
        }
        
        return hash;
    }
}

// MurmurHash3 高性能实现
namespace Murmur { // Murmur
    inline std::uint32_t rotl32(std::uint32_t x, std::int8_t r) {
        return (x << r) | (x >> (32 - r));
    }

    inline std::uint32_t fmix32(std::uint32_t h) {
        h ^= h >> 16;
        h *= 0x85EBCA6B;
        h ^= h >> 13;
        h *= 0xC2B2AE35;
        h ^= h >> 16;
        return h;
    }

    inline std::uint32_t hash32(const void* key, std::size_t len, std::uint32_t seed = 0) {
        const std::uint8_t* data = static_cast<const std::uint8_t*>(key);
        const std::size_t nblocks = len / 4;

        std::uint32_t h1 = seed;
        constexpr std::uint32_t c1 = 0xCC9E2D51;
        constexpr std::uint32_t c2 = 0x1B873593;

        // Body
        const std::uint32_t* blocks = reinterpret_cast<const std::uint32_t*>(data + nblocks * 4);
        for (std::size_t i = 0; i < nblocks; ++i) {
            std::uint32_t k1 = blocks[-static_cast<std::int32_t>(i) - 1];

            k1 *= c1;
            k1 = rotl32(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1 = rotl32(h1, 13);
            h1 = h1 * 5 + 0xE6546B64;
        }

        // Tail
        const std::uint8_t* tail = data + nblocks * 4;
        std::uint32_t k1 = 0;

        switch (len & 3) {
        case 3: k1 ^= tail[2] << 16; [[fallthrough]];
        case 2: k1 ^= tail[1] << 8;  [[fallthrough]];
        case 1: k1 ^= tail[0];
            k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2; h1 ^= k1;
        }

        // Finalization
        h1 ^= static_cast<std::uint32_t>(len);
        return fmix32(h1);
    }

} // namespace Murmur

} // namespace Hash

/**
 * 通用哈希函数模板类
 * 为常见类型提供高性能哈希实现
 */
template<typename T>
struct Hasher;

// 基础类型特化
template<>
struct Hasher<std::uint32_t> {
    std::size_t operator()(std::uint32_t value) const noexcept {
        return Hash::Murmur::hash32(&value, sizeof(value));
    }
};

template<>
struct Hasher<std::uint64_t> {
    std::size_t operator()(std::uint64_t value) const noexcept {
        return Hash::xxHash::hash64(&value, sizeof(value));
    }
};

template<>
struct Hasher<std::int32_t> {
    std::size_t operator()(std::int32_t value) const noexcept {
        return Hash::Murmur::hash32(&value, sizeof(value));
    }
};

template<>
struct Hasher<std::int64_t> {
    std::size_t operator()(std::int64_t value) const noexcept {
        return Hash::xxHash::hash64(&value, sizeof(value));
    }
};

// 字符串类型特化
template<>
struct Hasher<std::string> {
    std::size_t operator()(const std::string& str) const noexcept {
        return Hash::xxHash::hash64(str.data(), str.size());
    }
};

template<>
struct Hasher<std::string_view> {
    std::size_t operator()(std::string_view str) const noexcept {
        return Hash::xxHash::hash64(str.data(), str.size());
    }
};

template<>
struct Hasher<const char*> {
    std::size_t operator()(const char* str) const noexcept {
        return Hash::xxHash::hash64(str, std::strlen(str));
    }
};

// 指针类型特化
template<typename T>
struct Hasher<T*> {
    std::size_t operator()(T* ptr) const noexcept {
        std::uintptr_t value = reinterpret_cast<std::uintptr_t>(ptr);
        return Hash::xxHash::hash64(&value, sizeof(value));
    }
};

/**
 * 通用哈希函数，自动选择合适的算法
 */
template<typename T>
inline std::size_t hash_value(const T& value) {
    return Hasher<T>{}(value);
}

/**
 * 组合哈希值的辅助函数
 */
inline std::size_t hash_combine(std::size_t seed, std::size_t value) {
    // 使用 Boost 的组合算法
    return seed ^ (value + 0x9E3779B9 + (seed << 6) + (seed >> 2));
}

/**
 * 多值哈希组合
 */
template<typename T, typename... Args>
inline std::size_t hash_combine(std::size_t seed, const T& value, const Args&... args) {
    seed = hash_combine(seed, hash_value(value));
    if constexpr (sizeof...(args) > 0) {
        return hash_combine(seed, args...);
    }
    return seed;
}

/**
 * 创建多值组合哈希
 */
template<typename... Args>
inline std::size_t make_hash(const Args&... args) {
    std::size_t seed = 0;
    return hash_combine(seed, args...);
}

} // namespace Corona::Concurrent::Util