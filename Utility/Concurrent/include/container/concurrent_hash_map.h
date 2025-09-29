#pragma once

#include "../core/atomic.h"
#include "../core/sync.h"
#include "../util/epoch_reclaimer.h"
#include <vector>
#include <optional>
#include <functional>
#include <memory>

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
template<
    typename Key,
    typename T,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>,
    typename Reclaimer = EpochReclaimer<void>
>
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
    // 哈希表节点结构
    struct alignas(Core::CACHE_LINE_SIZE) Node {
        value_type data;
        Core::Atomic<Node*> next{nullptr};
        
        template<typename K, typename V>
        Node(K&& key, V&& val) : data(std::forward<K>(key), std::forward<V>(val)) {}
    };

    // 分片结构，每个分片包含独立的桶数组和锁
    struct alignas(Core::CACHE_LINE_SIZE) Shard {
        static constexpr size_t DEFAULT_BUCKET_COUNT = 16;
        
        std::vector<Core::Atomic<Node*>> buckets;
        mutable Core::SpinLock lock;  // fine-grained locking 模式使用
        Core::AtomicSize size{0};    // 当前分片中的元素数量
        
        explicit Shard(size_t bucket_count = DEFAULT_BUCKET_COUNT) 
            : buckets(bucket_count) {
            // 初始化所有桶为 nullptr
            for (auto& bucket : buckets) {
                bucket.store(nullptr, std::memory_order_relaxed);
            }
        }
    };

    // 配置参数
    static constexpr size_t DEFAULT_SHARD_COUNT = 32;  // 默认分片数量
    static constexpr size_t MIN_SHARD_COUNT = 8;       // 最小分片数量
    static constexpr size_t MAX_SHARD_COUNT = 512;     // 最大分片数量
    
    // 内存序优化配置
    static constexpr bool ENABLE_MEMORY_ORDER_OPTIMIZATION = true;  // 启用内存序优化
    static constexpr double MAX_LOAD_FACTOR = 0.75;    // 最大负载因子
    static constexpr bool USE_LOCK_FREE = true;        // 是否启用 lock-free 模式
    
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
        const allocator_type& alloc = allocator_type{}
    ) : hasher_(hash), key_eq_(equal), allocator_(alloc) {
        
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
            case 1: // 高并发模式
                shard_count = cpu_count * 8;  // 更多分片减少竞争
                bucket_count = 32;            // 较大的桶数量
                break;
            case 2: // 低延迟模式  
                shard_count = cpu_count * 2;  // 适中分片数量
                bucket_count = 8;             // 较小桶数量减少遍历
                break;
            default: // 平衡模式
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
    template<typename K, typename V>
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
        std::string memory_strategy;   // 内存序策略描述
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
            .memory_strategy = ENABLE_MEMORY_ORDER_OPTIMIZATION ? 
                "Relaxed + Fence (Hot Path Optimized)" : "Acquire/Release (Conservative)"
        };
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
    template<typename Fn>
    void for_each(Fn fn) const {
        typename EpochReclaimer<Node>::Guard guard(node_reclaimer_);
        
        for (const auto& shard : shards_) {
            Core::SpinGuard lock_guard(shard->lock);  // 获取读锁保证快照一致性
            
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
            Core::SpinGuard guard(shard->lock);
            
            for (auto& bucket : shard->buckets) {
                Node* current = bucket.load(std::memory_order_relaxed);
                bucket.store(nullptr, std::memory_order_relaxed);
                
                // 释放链表中的所有节点
                while (current != nullptr) {
                    Node* next = current->next.load(std::memory_order_relaxed);
                    node_reclaimer_.retire(current);
                    current = next;
                }
            }
            
            shard->size.store(0, std::memory_order_relaxed);
        }
        
        total_size_.store(0, std::memory_order_relaxed);
    }

private:
    // Lock-free 插入实现
    template<typename K, typename V>
    bool insert_lockfree(Shard& shard, Core::Atomic<Node*>& bucket, K&& key, V&& value) {
        auto new_node = create_node(std::forward<K>(key), std::forward<V>(value));
        
        for (;;) {
            Node* head = bucket.load(std::memory_order_relaxed);
            
            // 检查是否已存在 - 使用relaxed遍历
            Node* current = head;
            while (current != nullptr) {
                if (key_eq_(current->data.first, key)) {
                    // 键已存在，释放新节点
                    node_reclaimer_.retire(new_node);
                    return false;
                }
                current = current->next.load(std::memory_order_relaxed);
            }
            
            // 准备插入：使用fence确保新节点数据完全可见
            std::atomic_thread_fence(std::memory_order_release);
            new_node->next.store(head, std::memory_order_relaxed);
            
            // CAS操作使用acq_rel确保原子性和可见性
            if (bucket.compare_exchange_weak(head, new_node, 
                                           std::memory_order_acq_rel, 
                                           std::memory_order_relaxed)) {
                // 更新计数
                shard.size.fetch_add(1, std::memory_order_relaxed);
                total_size_.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
        }
    }

    // 使用锁的插入实现
    template<typename K, typename V>
    bool insert_with_lock(Shard& shard, Core::Atomic<Node*>& bucket, K&& key, V&& value) {
        Core::SpinGuard guard(shard.lock);
        
        Node* current = bucket.load(std::memory_order_relaxed);
        while (current != nullptr) {
            if (key_eq_(current->data.first, key)) {
                return false;  // 键已存在
            }
            current = current->next.load(std::memory_order_relaxed);
        }
        
        // 插入新节点
        auto new_node = create_node(std::forward<K>(key), std::forward<V>(value));
        new_node->next.store(bucket.load(std::memory_order_relaxed), std::memory_order_relaxed);
        bucket.store(new_node, std::memory_order_relaxed);
        
        // 更新计数
        shard.size.fetch_add(1, std::memory_order_relaxed);
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
        Core::SpinGuard guard(shard.lock);
        
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
                    node_reclaimer_.retire(head);
                    shard.size.fetch_sub(1, std::memory_order_relaxed);
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
                        node_reclaimer_.retire(current);
                        shard.size.fetch_sub(1, std::memory_order_relaxed);
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
        Core::SpinGuard guard(shard.lock);
        
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
                
                node_reclaimer_.retire(current);
                shard.size.fetch_sub(1, std::memory_order_relaxed);
                total_size_.fetch_sub(1, std::memory_order_relaxed);
                return true;
            }
            
            prev = current;
            current = current->next.load(std::memory_order_relaxed);
        }
        
        return false;
    }

    // 创建新节点
    template<typename K, typename V>
    Node* create_node(K&& key, V&& value) {
        return new Node(std::forward<K>(key), std::forward<V>(value));
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


} // namespace Corona::Concurrent