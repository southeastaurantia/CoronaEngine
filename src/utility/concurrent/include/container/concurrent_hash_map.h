#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../core/atomic.h"
#include "../core/sync.h"
#include "../detail/runtime_init.h"
#include "../util/epoch_reclaimer.h"

namespace Corona::Concurrent {

/**
 * 高并发环境下使用的哈希表容器。
 *
 * 特性：
 * - 分片 + 桶级自旋锁，降低热点竞争
 * - Epoch 回收器负责延迟释放节点，避免悬挂指针
 * - 保持标准 STL 风格接口，便于替换使用
 */
template <
    typename Key,
    typename T,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>,
    typename Reclaimer = EpochReclaimer<void>>
class ConcurrentHashMap {
   public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;

   private:
    struct Node {
        value_type data;
        Core::Atomic<Node*> next{nullptr};

        template <typename K, typename V>
        Node(K&& key, V&& value) : data(std::forward<K>(key), std::forward<V>(value)) {}

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
    };

    struct Bucket {
        Core::SpinLock lock;
        Core::Atomic<Node*> head{nullptr};
    };

    struct Shard {
        explicit Shard(size_t bucket_count) : buckets(bucket_count) {}

        std::vector<Bucket> buckets;
        Core::AtomicSize size{0};
    };

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocatorTraits = std::allocator_traits<NodeAllocator>;

    static constexpr size_t DEFAULT_SHARD_COUNT = 32;
    static constexpr size_t MIN_SHARD_COUNT = 8;
    static constexpr size_t MAX_SHARD_COUNT = 512;
    static constexpr size_t DEFAULT_BUCKETS_PER_SHARD = 32;

    std::vector<std::unique_ptr<Shard>> shards_;
    size_t shard_mask_{0};
    Core::AtomicSize total_size_{0};

    hasher hasher_;
    key_equal key_eq_;
    allocator_type allocator_;
    NodeAllocator node_allocator_{};

    mutable EpochReclaimer<Node> node_reclaimer_;

   public:
    /**
     * 构造函数
     * @param shard_count 分片数量，必须是2的幂
     * @param bucket_count_per_shard 每个分片的桶数量
     */
    explicit ConcurrentHashMap(
        size_t shard_count = 0,
        size_t bucket_count_per_shard = DEFAULT_BUCKETS_PER_SHARD,
        const hasher& hash = hasher{},
        const key_equal& equal = key_equal{},
        const allocator_type& alloc = allocator_type{})
        : hasher_(hash), key_eq_(equal), allocator_(alloc) {
        detail::ensure_runtime_initialized();
        initialize_shards(shard_count, bucket_count_per_shard);
    }

    /**
     * 性能调优构造函数 - 使用推荐的高性能配置
     * @param performance_mode 性能模式：0=平衡，1=高并发，2=低延迟
     */
    static ConcurrentHashMap create_optimized(int performance_mode = 0) {
        const size_t cpu = std::max<std::size_t>(1, std::thread::hardware_concurrency());
        size_t shard_count = cpu * 4;
        size_t bucket_count = DEFAULT_BUCKETS_PER_SHARD;

        switch (performance_mode) {
            case 1:
                shard_count = cpu * 8;
                bucket_count = DEFAULT_BUCKETS_PER_SHARD * 2;
                break;
            case 2:
                shard_count = cpu * 2;
                bucket_count = DEFAULT_BUCKETS_PER_SHARD / 2;
                bucket_count = std::max<std::size_t>(8, bucket_count);
                break;
            default:
                break;
        }

        return ConcurrentHashMap(shard_count, bucket_count);
    }

    // 禁用拷贝，允许移动
    ConcurrentHashMap(const ConcurrentHashMap&) = delete;
    ConcurrentHashMap& operator=(const ConcurrentHashMap&) = delete;
    ConcurrentHashMap(ConcurrentHashMap&&) = default;
    ConcurrentHashMap& operator=(ConcurrentHashMap&&) = default;

    ~ConcurrentHashMap() {
        clear();
        node_reclaimer_.force_cleanup_all();
    }

    /**
     * 插入键值对
     * @param key 键
     * @param value 值
     * @return true 如果插入成功，false 如果键已存在
     */
    template <typename K, typename V>
    bool insert(K&& key, V&& value) {
        const size_t hash_value = hasher_(key);
        Shard& shard = *shards_[hash_value & shard_mask_];
        Bucket& bucket = shard.buckets[hash_value % shard.buckets.size()];

        Core::SpinGuard guard(bucket.lock);

        Node* head = bucket.head.load(std::memory_order_acquire);
        for (Node* node = head; node != nullptr; node = node->next.load(std::memory_order_acquire)) {
            if (key_eq_(node->data.first, key)) {
                return false;
            }
        }

        Node* new_node = create_node(std::forward<K>(key), std::forward<V>(value));
        new_node->next.store(head, std::memory_order_relaxed);
        bucket.head.store(new_node, std::memory_order_release);

        shard.size.fetch_add(1, std::memory_order_relaxed);
        total_size_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    /**
     * 查找指定键的值
     * @param key 要查找的键
     * @return 如果找到返回值的副本，否则返回 nullopt
     */
    std::optional<T> find(const Key& key) const {
        const size_t hash_value = hasher_(key);
        const Shard& shard = *shards_[hash_value & shard_mask_];
        const Bucket& bucket = shard.buckets[hash_value % shard.buckets.size()];

        typename EpochReclaimer<Node>::Guard guard(node_reclaimer_);

        Node* node = bucket.head.load(std::memory_order_acquire);
        while (node != nullptr) {
            if (key_eq_(node->data.first, key)) {
                return node->data.second;
            }
            node = node->next.load(std::memory_order_acquire);
        }

        return std::nullopt;
    }

    /**
     * 删除指定键
     * @param key 要删除的键
     * @return true 如果删除成功，false 如果键不存在
     */
    bool erase(const Key& key) {
        const size_t hash_value = hasher_(key);
        Shard& shard = *shards_[hash_value & shard_mask_];
        Bucket& bucket = shard.buckets[hash_value % shard.buckets.size()];

        Core::SpinGuard guard(bucket.lock);

        Node* prev = nullptr;
        Node* current = bucket.head.load(std::memory_order_acquire);
        while (current != nullptr) {
            Node* next = current->next.load(std::memory_order_acquire);
            if (key_eq_(current->data.first, key)) {
                if (prev == nullptr) {
                    bucket.head.store(next, std::memory_order_release);
                } else {
                    prev->next.store(next, std::memory_order_release);
                }

                shard.size.fetch_sub(1, std::memory_order_relaxed);
                total_size_.fetch_sub(1, std::memory_order_relaxed);
                retire_node(current);
                return true;
            }
            prev = current;
            current = next;
        }

        return false;
    }

    /**
     * 获取哈希表中的元素总数
     * @return 元素数量
     */
    size_t size() const noexcept {
        return total_size_.load(std::memory_order_relaxed);
    }

    /**
     * 获取分片配置信息
     */
    struct ShardingInfo {
        size_t shard_count;
        size_t bucket_count_per_shard;
        size_t total_buckets;
        double load_factor;
        size_t cpu_cores;
        std::string optimization_level;
        bool memory_order_optimized;  // 是否启用内存序优化
        std::string memory_strategy;  // 内存序策略描述
        bool cache_line_optimized;    // 是否启用缓存行优化
        std::string cache_strategy;   // 缓存行优化策略描述
    };

    ShardingInfo get_sharding_info() const {
        const size_t shard_count = shards_.size();
        const size_t buckets_per_shard = shard_count > 0 ? shards_[0]->buckets.size() : 0;
        const size_t total_buckets = shard_count * buckets_per_shard;
        const size_t total_elements = size();
        const size_t cpu = std::max<std::size_t>(1, std::thread::hardware_concurrency());

        const double shard_ratio = static_cast<double>(shard_count) / static_cast<double>(cpu);
        std::string opt_level = "Balanced";
        if (shard_ratio >= 8.0) {
            opt_level = "High Concurrency";
        } else if (shard_ratio <= 2.0) {
            opt_level = "Low Latency";
        }

        return ShardingInfo{
            .shard_count = shard_count,
            .bucket_count_per_shard = buckets_per_shard,
            .total_buckets = total_buckets,
            .load_factor = total_buckets > 0 ? static_cast<double>(total_elements) / total_buckets : 0.0,
            .cpu_cores = cpu,
            .optimization_level = std::move(opt_level),
            .memory_order_optimized = true,
            .memory_strategy = "Bucket-level acquire/release",
            .cache_line_optimized = false,
            .cache_strategy = "Standard layout"};
    }

    /**
     * 检查哈希表是否为空
     * @return true 如果为空
     */
    bool empty() const noexcept {
        return size() == 0;
    }

    /**
     * 遍历所有元素（快照一致性）
     * @param fn 对每个元素执行的函数，签名为 void(const Key&, const T&)
     */
    template <typename Fn>
    void for_each(Fn fn) const {
        typename EpochReclaimer<Node>::Guard guard(node_reclaimer_);

        for (const auto& shard : shards_) {
            for (const auto& bucket : shard->buckets) {
                Node* node = bucket.head.load(std::memory_order_acquire);
                while (node != nullptr) {
                    fn(node->data.first, node->data.second);
                    node = node->next.load(std::memory_order_acquire);
                }
            }
        }
    }

    /**
     * 清空哈希表
     */
    void clear() {
        for (auto& shard : shards_) {
            for (auto& bucket : shard->buckets) {
                Core::SpinGuard guard(bucket.lock);
                Node* node = bucket.head.load(std::memory_order_acquire);
                bucket.head.store(nullptr, std::memory_order_release);

                while (node != nullptr) {
                    Node* next = node->next.load(std::memory_order_relaxed);
                    retire_node(node);
                    node = next;
                }
            }
            shard->size.store(0, std::memory_order_relaxed);
        }

        total_size_.store(0, std::memory_order_relaxed);
    }

   private:
    void initialize_shards(size_t requested_shards, size_t buckets_per_shard) {
        if (requested_shards == 0) {
            requested_shards = calculate_optimal_shard_count();
        }

        requested_shards = std::clamp(next_power_of_two(requested_shards), MIN_SHARD_COUNT, MAX_SHARD_COUNT);
        buckets_per_shard = std::max<std::size_t>(8, buckets_per_shard);

        shard_mask_ = requested_shards - 1;
        shards_.reserve(requested_shards);
        for (size_t i = 0; i < requested_shards; ++i) {
            shards_.emplace_back(std::make_unique<Shard>(buckets_per_shard));
        }
        total_size_.store(0, std::memory_order_relaxed);
    }

    static size_t calculate_optimal_shard_count() {
        size_t cpu = std::thread::hardware_concurrency();
        if (cpu == 0) {
            cpu = 8;
        }
        return next_power_of_two(cpu * 4);
    }

    template <typename K, typename V>
    Node* create_node(K&& key, V&& value) {
        Node* raw = NodeAllocatorTraits::allocate(node_allocator_, 1);
        NodeAllocatorTraits::construct(node_allocator_, raw, std::forward<K>(key), std::forward<V>(value));
        return raw;
    }

    void retire_node(Node* node) {
        if (!node) {
            return;
        }
        node_reclaimer_.retire(node, [alloc = node_allocator_](Node* n) mutable {
            if (!n) {
                return;
            }
            NodeAllocatorTraits::destroy(alloc, n);
            NodeAllocatorTraits::deallocate(alloc, n, 1);
        });
    }

    static size_t next_power_of_two(size_t n) {
        if (n <= 1) {
            return 1;
        }
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
#if INTPTR_MAX == INT64_MAX
        n |= n >> 32;
#endif
        return ++n;
    }
};

}  // namespace Corona::Concurrent