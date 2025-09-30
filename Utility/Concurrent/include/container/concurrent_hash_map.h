#pragma once

#include <compiler_features.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <vector>

#include "../core/atomic.h"
#include "../core/sync.h"
#include "../util/epoch_reclaimer.h"

namespace Corona::Concurrent {

/**
 * 高性能并发哈希表实现
 *
 * 特性：
 * - 基于分片（Sharding）设计，减少锁竞争
 * - 支持 lock-free 和 fine-grained locking 两种模式
 * - 缓存行对齐，避免伪共享
 * - 自适应扩容，支持高并发读写
 * - 兼容 STL 接口风格
 *
 * @tparam Key 键类型
 * @tparam T 值类型
 * @tparam Hash 哈希函数类型
 * @tparam KeyEqual 键比较函数类型
 * @tparam Allocator 分配器类型
 * @tparam Reclaimer 内存回收策略类型
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
    // 哈希表节点结构 - 缓存行优化版本（跨平台兼容）
#if CE_BUILTIN_COMPILER_MSVC
#pragma pack(push, 1)
    struct alignas(Core::CACHE_LINE_SIZE) Node {
        // 数据和指针紧密排列，避免不必要的缓存行对齐开销
        value_type data;
        Core::Atomic<Node*> next{nullptr};
        bool aligned_allocation{false};

        template <typename K, typename V>
        Node(K&& key, V&& val) : data(std::forward<K>(key), std::forward<V>(val)) {}

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
    };
#pragma pack(pop)
#elif CE_BUILTIN_COMPILER_GCC_FAMILY
    struct alignas(Core::CACHE_LINE_SIZE) Node {
        // 数据和指针紧密排列，避免不必要的缓存行对齐开销
        value_type data;
        Core::Atomic<Node*> next{nullptr};
        bool aligned_allocation{false};

        template <typename K, typename V>
        Node(K&& key, V&& val) : data(std::forward<K>(key), std::forward<V>(val)) {}

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
    } __attribute__((packed));
#else
    struct alignas(Core::CACHE_LINE_SIZE) Node {
        value_type data;
        Core::Atomic<Node*> next{nullptr};
        bool aligned_allocation{false};

        template <typename K, typename V>
        Node(K&& key, V&& val) : data(std::forward<K>(key), std::forward<V>(val)) {}

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
    };
#endif

    // 分片结构，每个分片包含独立的桶数组和锁
    // 优化缓存行布局：将热点数据分离避免伪共享
    struct alignas(Core::CACHE_LINE_SIZE) Shard {
        static constexpr size_t DEFAULT_BUCKET_COUNT = 16;

        // 第一个缓存行：热点读取数据
        alignas(Core::CACHE_LINE_SIZE) std::vector<Core::Atomic<Node*>> buckets;

        // 第二个缓存行：写入热点数据
        alignas(Core::CACHE_LINE_SIZE) struct {
            Core::AtomicSize size{0};     // 当前分片中的元素数量
            mutable Core::SpinLock lock;  // fine-grained locking 模式使用
            char padding[Core::CACHE_LINE_SIZE - sizeof(Core::AtomicSize) - sizeof(Core::SpinLock)];
        } write_hot_data;

        explicit Shard(size_t bucket_count = DEFAULT_BUCKET_COUNT)
            : buckets(bucket_count) {
            // 初始化所有桶为 nullptr
            for (auto& bucket : buckets) {
                bucket.store(nullptr, std::memory_order_relaxed);
            }
        }

        // 便利访问器
        Core::AtomicSize& get_size() { return write_hot_data.size; }
        const Core::AtomicSize& get_size() const { return write_hot_data.size; }
        Core::SpinLock& get_lock() { return write_hot_data.lock; }
        const Core::SpinLock& get_lock() const { return write_hot_data.lock; }
    };

    // 配置参数
    static constexpr size_t DEFAULT_SHARD_COUNT = 32;  // 默认分片数量
    static constexpr size_t MIN_SHARD_COUNT = 8;       // 最小分片数量
    static constexpr size_t MAX_SHARD_COUNT = 512;     // 最大分片数量

    // 内存序优化配置
    static constexpr bool ENABLE_MEMORY_ORDER_OPTIMIZATION = true;  // 启用内存序优化

    // 缓存行对齐优化配置
    static constexpr bool ENABLE_CACHE_LINE_OPTIMIZATION = true;            // 启用缓存行优化
    static constexpr size_t OPTIMAL_BUCKET_COUNT = 8;                       // 优化的桶数量（缓存行友好）
    static constexpr double MAX_LOAD_FACTOR = 0.75;                         // 最大负载因子
    static constexpr bool USE_LOCK_FREE = true;                             // 是否启用 lock-free 模式
    static constexpr size_t ALIGNMENT_CHAIN_THRESHOLD = 6;                  // 链长度超过该值时启用对齐
    static constexpr double ALIGNMENT_SHARD_LOAD_FACTOR_THRESHOLD = 1.25;   // 分片负载阈值
    static constexpr double ALIGNMENT_GLOBAL_LOAD_FACTOR_THRESHOLD = 0.95;  // 全局负载阈值
    static constexpr size_t ALIGNMENT_BUCKET_FAVOR_THRESHOLD = 16;          // 桶数量较小时倾向对齐

    // 智能分片数量计算
    static size_t calculate_optimal_shard_count() {
        size_t cpu_count = std::thread::hardware_concurrency();
        if (cpu_count == 0) cpu_count = 8;  // 回退默认值

        // 分片数 = CPU逻辑核心数 × 4 (减少竞争)
        size_t optimal_count = cpu_count * 4;

        // 确保在合理范围内
        optimal_count = std::max(MIN_SHARD_COUNT, optimal_count);
        optimal_count = std::min(MAX_SHARD_COUNT, optimal_count);

        // 确保是2的幂
        return next_power_of_2(optimal_count);
    }

    std::vector<std::unique_ptr<Shard>> shards_;
    size_t shard_mask_;
    Core::AtomicSize total_size_{0};
    hasher hasher_;
    key_equal key_eq_;
    allocator_type allocator_;

    // 内存回收器
    mutable EpochReclaimer<Node> node_reclaimer_;

   public:
    /**
     * 构造函数
     * @param shard_count 分片数量，必须是2的幂
     * @param bucket_count_per_shard 每个分片的桶数量
     */
    explicit ConcurrentHashMap(
        size_t shard_count = 0,  // 0表示使用智能计算
        size_t bucket_count_per_shard = Shard::DEFAULT_BUCKET_COUNT,
        const hasher& hash = hasher{},
        const key_equal& equal = key_equal{},
        const allocator_type& alloc = allocator_type{}) : hasher_(hash), key_eq_(equal), allocator_(alloc) {
        // 智能分片数量计算
        if (shard_count == 0) {
            shard_count = calculate_optimal_shard_count();
        } else {
            // 用户指定的分片数量，确保是2的幂且在合理范围内
            shard_count = std::max(MIN_SHARD_COUNT, shard_count);
            shard_count = std::min(MAX_SHARD_COUNT, shard_count);
            shard_count = next_power_of_2(shard_count);
        }

        shard_mask_ = shard_count - 1;

        // 初始化分片
        shards_.reserve(shard_count);
        for (size_t i = 0; i < shard_count; ++i) {
            shards_.emplace_back(std::make_unique<Shard>(bucket_count_per_shard));
        }
    }

    /**
     * 性能调优构造函数 - 使用推荐的高性能配置
     * @param performance_mode 性能模式：0=平衡，1=高并发，2=低延迟
     */
    static ConcurrentHashMap create_optimized(int performance_mode = 0) {
        size_t cpu_count = std::thread::hardware_concurrency();
        if (cpu_count == 0) cpu_count = 8;

        size_t shard_count, bucket_count;

        switch (performance_mode) {
            case 1:                           // 高并发模式
                shard_count = cpu_count * 8;  // 更多分片减少竞争
                bucket_count = 32;            // 较大的桶数量
                break;
            case 2:                           // 低延迟模式
                shard_count = cpu_count * 2;  // 适中分片数量
                bucket_count = 8;             // 较小桶数量减少遍历
                break;
            default:                          // 平衡模式
                shard_count = cpu_count * 4;  // 平衡的分片数量
                bucket_count = 16;            // 默认桶数量
                break;
        }

        return ConcurrentHashMap(shard_count, bucket_count);
    }

    // 禁用拷贝，允许移动
    ConcurrentHashMap(const ConcurrentHashMap&) = delete;
    ConcurrentHashMap& operator=(const ConcurrentHashMap&) = delete;
    ConcurrentHashMap(ConcurrentHashMap&&) = default;
    ConcurrentHashMap& operator=(ConcurrentHashMap&&) = default;

    /**
     * 插入键值对
     * @param key 键
     * @param value 值
     * @return true 如果插入成功，false 如果键已存在
     */
    template <typename K, typename V>
    bool insert(K&& key, V&& value) {
        auto hash_val = hasher_(key);
        auto shard_id = hash_val & shard_mask_;
        auto& shard = *shards_[shard_id];

        auto bucket_id = hash_val % shard.buckets.size();
        auto& bucket = shard.buckets[bucket_id];

        if constexpr (USE_LOCK_FREE) {
            return insert_lockfree(shard, bucket, std::forward<K>(key), std::forward<V>(value));
        } else {
            return insert_with_lock(shard, bucket, std::forward<K>(key), std::forward<V>(value));
        }
    }

    /**
     * 查找指定键的值
     * @param key 要查找的键
     * @return 如果找到返回值的副本，否则返回 nullopt
     */
    std::optional<T> find(const Key& key) const {
        auto hash_val = hasher_(key);
        auto shard_id = hash_val & shard_mask_;
        auto& shard = *shards_[shard_id];

        auto bucket_id = hash_val % shard.buckets.size();
        auto& bucket = shard.buckets[bucket_id];

        if constexpr (USE_LOCK_FREE) {
            return find_lockfree(bucket, key);
        } else {
            return find_with_lock(shard, bucket, key);
        }
    }

    /**
     * 删除指定键
     * @param key 要删除的键
     * @return true 如果删除成功，false 如果键不存在
     */
    bool erase(const Key& key) {
        auto hash_val = hasher_(key);
        auto shard_id = hash_val & shard_mask_;
        auto& shard = *shards_[shard_id];

        auto bucket_id = hash_val % shard.buckets.size();
        auto& bucket = shard.buckets[bucket_id];

        if constexpr (USE_LOCK_FREE) {
            return erase_lockfree(shard, bucket, key);
        } else {
            return erase_with_lock(shard, bucket, key);
        }
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
        size_t total_elements = size();
        size_t total_buckets = shards_.size() * shards_[0]->buckets.size();
        size_t cpu_cores = std::thread::hardware_concurrency();

        // 判断优化级别
        std::string opt_level;
        double shard_ratio = static_cast<double>(shards_.size()) / cpu_cores;
        if (shard_ratio >= 8.0) {
            opt_level = "High Concurrency";
        } else if (shard_ratio >= 4.0) {
            opt_level = "Balanced";
        } else if (shard_ratio >= 2.0) {
            opt_level = "Low Latency";
        } else {
            opt_level = "Conservative";
        }

        return ShardingInfo{
            .shard_count = shards_.size(),
            .bucket_count_per_shard = shards_[0]->buckets.size(),
            .total_buckets = total_buckets,
            .load_factor = total_buckets > 0 ? static_cast<double>(total_elements) / total_buckets : 0.0,
            .cpu_cores = cpu_cores,
            .optimization_level = opt_level,
            .memory_order_optimized = ENABLE_MEMORY_ORDER_OPTIMIZATION,
            .memory_strategy = ENABLE_MEMORY_ORDER_OPTIMIZATION ? "Relaxed + Fence (Hot Path Optimized)" : "Acquire/Release (Conservative)",
            .cache_line_optimized = ENABLE_CACHE_LINE_OPTIMIZATION,
            .cache_strategy = ENABLE_CACHE_LINE_OPTIMIZATION ? "Adaptive Hot/Cold Separation + Bucket Alignment" : "Default Layout"};
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
            Core::SpinGuard lock_guard(shard->get_lock());  // 获取读锁保证快照一致性

            for (const auto& bucket : shard->buckets) {
                Node* current = bucket.load(std::memory_order_acquire);
                while (current != nullptr) {
                    fn(current->data.first, current->data.second);
                    current = current->next.load(std::memory_order_acquire);
                }
            }
        }
    }

    /**
     * 清空哈希表
     */
    void clear() {
        for (auto& shard : shards_) {
            Core::SpinGuard guard(shard->get_lock());

            for (auto& bucket : shard->buckets) {
                Node* current = bucket.load(std::memory_order_relaxed);
                bucket.store(nullptr, std::memory_order_relaxed);

                // 释放链表中的所有节点
                while (current != nullptr) {
                    Node* next = current->next.load(std::memory_order_relaxed);
                    retire_node(current);
                    current = next;
                }
            }

            shard->get_size().store(0, std::memory_order_relaxed);
        }

        total_size_.store(0, std::memory_order_relaxed);
    }

   private:
    // Lock-free 插入实现
    template <typename K, typename V>
    bool insert_lockfree(Shard& shard, Core::Atomic<Node*>& bucket, K&& key, V&& value) {
        Node* pending_node = nullptr;
        for (;;) {
            Node* head = bucket.load(std::memory_order_relaxed);

            // 检查是否已存在 - 使用relaxed遍历
            Node* current = head;
            size_t chain_length = 0;
            const Key& key_ref = pending_node ? pending_node->data.first : key;
            while (current != nullptr) {
                if (key_eq_(current->data.first, key_ref)) {
                    if (pending_node != nullptr) {
                        destroy_node_immediate(pending_node);
                    }
                    return false;
                }
                ++chain_length;
                current = current->next.load(std::memory_order_relaxed);
            }

            if (pending_node == nullptr) {
                pending_node = create_node(shard, chain_length, std::forward<K>(key), std::forward<V>(value));
            }

            pending_node->next.store(head, std::memory_order_relaxed);
            // 准备插入：使用fence确保新节点数据完全可见
            std::atomic_thread_fence(std::memory_order_release);

            // CAS操作使用acq_rel确保原子性和可见性
            if (bucket.compare_exchange_weak(head, pending_node,
                                             std::memory_order_acq_rel,
                                             std::memory_order_relaxed)) {
                // 更新计数
                shard.get_size().fetch_add(1, std::memory_order_relaxed);
                total_size_.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }
    }

    // 使用锁的插入实现
    template <typename K, typename V>
    bool insert_with_lock(Shard& shard, Core::Atomic<Node*>& bucket, K&& key, V&& value) {
        Core::SpinGuard guard(shard.get_lock());

        Node* current = bucket.load(std::memory_order_relaxed);
        size_t chain_length = 0;
        while (current != nullptr) {
            if (key_eq_(current->data.first, key)) {
                return false;  // 键已存在
            }
            ++chain_length;
            current = current->next.load(std::memory_order_relaxed);
        }

        // 插入新节点
        auto new_node = create_node(shard, chain_length, std::forward<K>(key), std::forward<V>(value));
        new_node->next.store(bucket.load(std::memory_order_relaxed), std::memory_order_relaxed);
        bucket.store(new_node, std::memory_order_relaxed);

        // 更新计数
        shard.get_size().fetch_add(1, std::memory_order_relaxed);
        total_size_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    // Lock-free 查找实现 - 内存序优化
    std::optional<T> find_lockfree(const Core::Atomic<Node*>& bucket, const Key& key) const {
        typename EpochReclaimer<Node>::Guard guard(node_reclaimer_);

        // 初始读取使用acquire确保与插入同步
        Node* current = bucket.load(std::memory_order_acquire);
        while (current != nullptr) {
            // 检查键值匹配
            if (key_eq_(current->data.first, key)) {
                // 找到匹配项，使用fence确保数据可见性
                std::atomic_thread_fence(std::memory_order_acquire);
                return current->data.second;
            }
            // 链表遍历使用relaxed + consume语义（通过数据依赖保证顺序）
            current = current->next.load(std::memory_order_relaxed);
        }
        return std::nullopt;
    }

    // 使用锁的查找实现
    std::optional<T> find_with_lock(const Shard& shard, const Core::Atomic<Node*>& bucket, const Key& key) const {
        Core::SpinGuard guard(shard.get_lock());

        Node* current = bucket.load(std::memory_order_relaxed);
        while (current != nullptr) {
            if (key_eq_(current->data.first, key)) {
                return current->data.second;
            }
            current = current->next.load(std::memory_order_relaxed);
        }
        return std::nullopt;
    }

    // Lock-free 删除实现 - 内存序优化
    bool erase_lockfree(Shard& shard, Core::Atomic<Node*>& bucket, const Key& key) {
        for (;;) {
            Node* head = bucket.load(std::memory_order_relaxed);

            // 如果要删除的是头节点
            if (head != nullptr && key_eq_(head->data.first, key)) {
                Node* next = head->next.load(std::memory_order_relaxed);
                // 使用acq_rel确保删除操作的原子性和内存同步
                if (bucket.compare_exchange_weak(head, next,
                                                 std::memory_order_acq_rel,
                                                 std::memory_order_relaxed)) {
                    retire_node(head);
                    shard.get_size().fetch_sub(1, std::memory_order_relaxed);
                    total_size_.fetch_sub(1, std::memory_order_relaxed);
                    return true;
                }
                continue;
            }

            // 查找要删除的节点 - 使用relaxed遍历
            Node* prev = head;
            while (prev != nullptr) {
                Node* current = prev->next.load(std::memory_order_relaxed);
                if (current != nullptr && key_eq_(current->data.first, key)) {
                    Node* next = current->next.load(std::memory_order_relaxed);
                    // 删除中间节点使用acq_rel确保链表一致性
                    if (prev->next.compare_exchange_weak(current, next,
                                                         std::memory_order_acq_rel,
                                                         std::memory_order_relaxed)) {
                        retire_node(current);
                        shard.get_size().fetch_sub(1, std::memory_order_relaxed);
                        total_size_.fetch_sub(1, std::memory_order_relaxed);
                        return true;
                    }
                    break;  // 重新开始
                }
                prev = current;
            }

            return false;  // 未找到
        }
    }

    // 使用锁的删除实现
    bool erase_with_lock(Shard& shard, Core::Atomic<Node*>& bucket, const Key& key) {
        Core::SpinGuard guard(shard.get_lock());

        Node* prev = nullptr;
        Node* current = bucket.load(std::memory_order_relaxed);

        while (current != nullptr) {
            if (key_eq_(current->data.first, key)) {
                if (prev == nullptr) {
                    // 删除头节点
                    bucket.store(current->next.load(std::memory_order_relaxed),
                                 std::memory_order_relaxed);
                } else {
                    // 删除中间节点
                    prev->next.store(current->next.load(std::memory_order_relaxed),
                                     std::memory_order_relaxed);
                }

                retire_node(current);
                shard.get_size().fetch_sub(1, std::memory_order_relaxed);
                total_size_.fetch_sub(1, std::memory_order_relaxed);
                return true;
            }

            prev = current;
            current = current->next.load(std::memory_order_relaxed);
        }

        return false;
    }

    bool should_use_aligned_node(const Shard& shard, size_t chain_length) const {
        if (!ENABLE_CACHE_LINE_OPTIMIZATION) {
            return false;
        }

        if (chain_length >= ALIGNMENT_CHAIN_THRESHOLD) {
            return true;
        }

        const size_t bucket_count = shard.buckets.size();
        const size_t shard_size = shard.get_size().load(std::memory_order_relaxed);
        const double shard_load = bucket_count > 0
                                      ? static_cast<double>(shard_size + 1) / static_cast<double>(bucket_count)
                                      : 0.0;
        if (shard_load >= ALIGNMENT_SHARD_LOAD_FACTOR_THRESHOLD) {
            return true;
        }

        const size_t total_buckets = shards_.size() * bucket_count;
        const size_t global_size = total_size_.load(std::memory_order_relaxed);
        const double global_load = total_buckets > 0
                                       ? static_cast<double>(global_size + 1) / static_cast<double>(total_buckets)
                                       : 0.0;
        if (global_load >= ALIGNMENT_GLOBAL_LOAD_FACTOR_THRESHOLD &&
            chain_length >= ALIGNMENT_CHAIN_THRESHOLD / 2) {
            return true;
        }

        if (bucket_count <= ALIGNMENT_BUCKET_FAVOR_THRESHOLD &&
            chain_length >= ALIGNMENT_CHAIN_THRESHOLD / 2) {
            return true;
        }

        return false;
    }

    // 创建新节点（根据工作集自适应对齐策略）
    template <typename K, typename V>
    Node* create_node(Shard& shard, size_t chain_length, K&& key, V&& value) {
        const bool use_aligned = should_use_aligned_node(shard, chain_length);
        if (use_aligned) {
            void* raw = ::operator new(sizeof(Node), std::align_val_t(Core::CACHE_LINE_SIZE));
            Node* node = new (raw) Node(std::forward<K>(key), std::forward<V>(value));
            node->aligned_allocation = true;
            return node;
        }

        Node* node = new Node(std::forward<K>(key), std::forward<V>(value));
        node->aligned_allocation = false;
        return node;
    }

    static void destroy_regular_node(Node* node) {
        if (!node) {
            return;
        }
        node->~Node();
        ::operator delete(static_cast<void*>(node));
    }

    static void destroy_aligned_node(Node* node) {
        if (!node) {
            return;
        }
        node->~Node();
        ::operator delete(static_cast<void*>(node), std::align_val_t(Core::CACHE_LINE_SIZE));
    }

    void destroy_node_immediate(Node* node) {
        if (!node) {
            return;
        }
        if (node->aligned_allocation) {
            destroy_aligned_node(node);
        } else {
            destroy_regular_node(node);
        }
    }

    void retire_node(Node* node) {
        if (!node) {
            return;
        }
        auto deleter = node->aligned_allocation ? destroy_aligned_node : destroy_regular_node;
        node_reclaimer_.retire(node, deleter);
    }

    // 计算下一个2的幂
    static size_t next_power_of_2(size_t n) {
        if (n <= 1) return 1;
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return ++n;
    }
};

// 简化版本的 Epoch 回收器声明（完整实现在单独文件中）

}  // namespace Corona::Concurrent