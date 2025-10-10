#pragma once

#include <cstdint>

#include "atomic.h"
#include "thread.h"


namespace Corona::Concurrent::Core {

/**
 * 自旋锁实现
 * 适用于临界区很短的场景
 */
class SpinLock {
   private:
    AtomicBool locked_{false};

   public:
    SpinLock() = default;

    // 禁止拷贝和移动
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    /**
     * 尝试获取锁（非阻塞）
     */
    bool try_lock() noexcept {
        return !locked_.exchange(true, std::memory_order_acquire);
    }

    /**
     * 获取锁（阻塞直到成功）
     */
    void lock() noexcept {
        SpinWait spinner;
        while (locked_.exchange(true, std::memory_order_acquire)) {
            // 自旋等待，使用指数退避
            while (locked_.load(std::memory_order_relaxed)) {
                spinner.spin_once();
            }
        }
    }

    /**
     * 释放锁
     */
    void unlock() noexcept {
        locked_.store(false, std::memory_order_release);
    }

    /**
     * 检查锁是否被持有
     */
    bool is_locked() const noexcept {
        return locked_.load(std::memory_order_relaxed);
    }
};

/**
 * 票据锁实现
 * 保证获取锁的公平性（FIFO）
 */
class TicketLock {
   private:
    AtomicUInt64 next_ticket_{0};
    AtomicUInt64 serving_ticket_{0};

   public:
    TicketLock() = default;

    TicketLock(const TicketLock&) = delete;
    TicketLock& operator=(const TicketLock&) = delete;
    TicketLock(TicketLock&&) = delete;
    TicketLock& operator=(TicketLock&&) = delete;

    /**
     * 获取锁
     */
    void lock() noexcept {
        std::uint64_t ticket = next_ticket_++;
        SpinWait spinner;

        while (serving_ticket_.load(std::memory_order_acquire) != ticket) {
            spinner.spin_once();
        }
    }

    /**
     * 尝试获取锁
     */
    bool try_lock() noexcept {
        std::uint64_t current_serving = serving_ticket_.load(std::memory_order_acquire);
        std::uint64_t current_next = next_ticket_.load(std::memory_order_relaxed);

        if (current_serving == current_next) {
            return next_ticket_.compare_exchange_weak(current_next, current_next + 1,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed);
        }
        return false;
    }

    /**
     * 释放锁
     */
    void unlock() noexcept {
        serving_ticket_++;
    }

    /**
     * 获取等待队列长度
     */
    std::uint64_t queue_length() const noexcept {
        return next_ticket_.load(std::memory_order_relaxed) -
               serving_ticket_.load(std::memory_order_relaxed);
    }
};

/**
 * 序列锁实现
 * 适用于读多写少的场景，读者无锁，写者独占
 */
class SeqLock {
   private:
    AtomicUInt64 sequence_{0};

   public:
    SeqLock() = default;

    SeqLock(const SeqLock&) = delete;
    SeqLock& operator=(const SeqLock&) = delete;
    SeqLock(SeqLock&&) = delete;
    SeqLock& operator=(SeqLock&&) = delete;

    /**
     * 写者获取锁
     */
    void write_lock() noexcept {
        std::uint64_t expected = sequence_.load(std::memory_order_relaxed);
        while (expected & 1 || !sequence_.compare_exchange_weak(expected, expected + 1,
                                                                std::memory_order_acquire,
                                                                std::memory_order_relaxed)) {
            cpu_relax();
            expected = sequence_.load(std::memory_order_relaxed);
        }
    }

    /**
     * 写者释放锁
     */
    void write_unlock() noexcept {
        sequence_.fetch_add(1, std::memory_order_release);
    }

    /**
     * 读者开始读取（获取序列号）
     */
    std::uint64_t read_begin() const noexcept {
        std::uint64_t seq;
        do {
            seq = sequence_.load(std::memory_order_acquire);
        } while (seq & 1);  // 等待写操作完成
        return seq;
    }

    /**
     * 读者完成读取（验证序列号）
     */
    bool read_end(std::uint64_t seq) const noexcept {
        acquire_fence();
        return sequence_.load(std::memory_order_acquire) == seq;
    }

    /**
     * 便利的读取重试宏
     * 使用方法：
     * SEQ_LOCK_READ_RETRY(seqlock) {
     *     // 读取共享数据
     * } SEQ_LOCK_READ_RETRY_END;
     */
    class ReadTransaction {
       private:
        const SeqLock& lock_;
        std::uint64_t seq_;

       public:
        explicit ReadTransaction(const SeqLock& lock) : lock_(lock) {
            seq_ = lock_.read_begin();
        }

        bool valid() const noexcept {
            return lock_.read_end(seq_);
        }

        void retry() noexcept {
            seq_ = lock_.read_begin();
        }
    };

    ReadTransaction begin_read() const {
        return ReadTransaction(*this);
    }
};

/**
 * 读写锁实现
 * 支持多个读者或单个写者
 */
class RWLock {
   private:
    static constexpr std::uint32_t READER_MASK = 0x7FFFFFFF;
    static constexpr std::uint32_t WRITER_FLAG = 0x80000000;

    AtomicUInt32 state_{0};  // 低31位：读者计数，高1位：写者标记

   public:
    RWLock() = default;

    RWLock(const RWLock&) = delete;
    RWLock& operator=(const RWLock&) = delete;
    RWLock(RWLock&&) = delete;
    RWLock& operator=(RWLock&&) = delete;

    /**
     * 读者获取锁
     */
    void read_lock() noexcept {
        SpinWait spinner;
        while (true) {
            std::uint32_t current = state_.load(std::memory_order_acquire);

            // 如果有写者或读者已满，等待
            if ((current & WRITER_FLAG) || (current & READER_MASK) == READER_MASK) {
                spinner.spin_once();
                continue;
            }

            // 尝试增加读者计数
            if (state_.compare_exchange_weak(current, current + 1,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed)) {
                break;
            }
        }
    }

    /**
     * 读者尝试获取锁
     */
    bool try_read_lock() noexcept {
        std::uint32_t current = state_.load(std::memory_order_acquire);

        if ((current & WRITER_FLAG) || (current & READER_MASK) == READER_MASK) {
            return false;
        }

        return state_.compare_exchange_strong(current, current + 1,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed);
    }

    /**
     * 读者释放锁
     */
    void read_unlock() noexcept {
        state_.fetch_sub(1, std::memory_order_release);
    }

    /**
     * 写者获取锁
     */
    void write_lock() noexcept {
        SpinWait spinner;
        while (true) {
            std::uint32_t current = state_.load(std::memory_order_acquire);

            // 如果有其他写者，等待
            if (current & WRITER_FLAG) {
                spinner.spin_once();
                continue;
            }

            // 尝试设置写者标记
            if (state_.compare_exchange_weak(current, current | WRITER_FLAG,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed)) {
                // 等待所有读者完成
                while ((state_.load(std::memory_order_acquire) & READER_MASK) != 0) {
                    spinner.spin_once();
                }
                break;
            }
        }
    }

    /**
     * 写者尝试获取锁
     */
    bool try_write_lock() noexcept {
        std::uint32_t expected = 0;
        return state_.compare_exchange_strong(expected, WRITER_FLAG,
                                              std::memory_order_acquire,
                                              std::memory_order_relaxed);
    }

    /**
     * 写者释放锁
     */
    void write_unlock() noexcept {
        state_.fetch_and(~WRITER_FLAG, std::memory_order_release);
    }

    /**
     * 获取当前读者数量
     */
    std::uint32_t reader_count() const noexcept {
        return state_.load(std::memory_order_relaxed) & READER_MASK;
    }

    /**
     * 检查是否有写者
     */
    bool has_writer() const noexcept {
        return (state_.load(std::memory_order_relaxed) & WRITER_FLAG) != 0;
    }
};

/**
 * RAII 锁守卫
 */
template <typename Lock>
class LockGuard {
   private:
    Lock& lock_;

   public:
    explicit LockGuard(Lock& lock) : lock_(lock) {
        lock_.lock();
    }

    ~LockGuard() {
        lock_.unlock();
    }

    LockGuard(const LockGuard&) = delete;
    LockGuard& operator=(const LockGuard&) = delete;
    LockGuard(LockGuard&&) = delete;
    LockGuard& operator=(LockGuard&&) = delete;
};

/**
 * 读锁守卫
 */
template <typename RWLock>
class ReadGuard {
   private:
    RWLock& lock_;

   public:
    explicit ReadGuard(RWLock& lock) : lock_(lock) {
        lock_.read_lock();
    }

    ~ReadGuard() {
        lock_.read_unlock();
    }

    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;
    ReadGuard(ReadGuard&&) = delete;
    ReadGuard& operator=(ReadGuard&&) = delete;
};

/**
 * 写锁守卫
 */
template <typename RWLock>
class WriteGuard {
   private:
    RWLock& lock_;

   public:
    explicit WriteGuard(RWLock& lock) : lock_(lock) {
        lock_.write_lock();
    }

    ~WriteGuard() {
        lock_.write_unlock();
    }

    WriteGuard(const WriteGuard&) = delete;
    WriteGuard& operator=(const WriteGuard&) = delete;
    WriteGuard(WriteGuard&&) = delete;
    WriteGuard& operator=(WriteGuard&&) = delete;
};

// 便利的类型别名和宏
using SpinGuard = LockGuard<SpinLock>;
using TicketGuard = LockGuard<TicketLock>;

#define SEQ_LOCK_READ_RETRY(seqlock)               \
    do {                                           \
        auto __seq_trans = (seqlock).begin_read(); \
        do {
#define SEQ_LOCK_READ_RETRY_END                    \
    }                                              \
    while (!__seq_trans.valid());                  \
    if (!__seq_trans.valid()) __seq_trans.retry(); \
    }                                              \
    while (false)

}  // namespace Corona::Concurrent::Core