#pragma once
#include <functional>
// 依赖 oneTBB 并发队列实现线程安全命令投递
#include <oneapi/tbb.h>

namespace Corona
{
    // 线程安全命令队列：支持跨线程投递与消费，常用于系统线程之间通信
    class SafeCommandQueue
    {
      public:
        using Command = std::function<void()>;

        /**
         * @brief 原始入队方法，接受一个已封装的命令。
         * @param cmd 一个 std::function<void()> 对象。
         */
        void enqueue(Command cmd)
        {
            safe_queue.push(std::move(cmd));
        }

        /**
         * @brief 封装了 std::bind 的便利方法。
         * 接受一个可调用对象（如函数、成员函数指针）及其参数，
         * 并将它们绑定为一个命令后入队。
         *
         * @tparam F 可调用对象的类型。
         * @tparam Args 参数包的类型。
         * @param f 可调用对象。
         * @param args 传递给可调用对象的参数。
         */
        template <typename F, typename... Args>
        void enqueue(F &&f, Args &&...args)
        {
            // 使用 std::bind 将函数和参数绑定在一起
            // 使用 std::forward 实现完美转发，保留参数的原始值类别（左值/右值）
            Command cmd = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            safe_queue.push(std::move(cmd));
        }

        // 从队列中取出一个命令并执行（无阻塞）
        bool try_execute()
        {
            if (Command cmd; safe_queue.try_pop(cmd))
            {
                cmd();
                return true;
            }
            return false;
        }

        // 检查队列是否为空（无锁快查）
        [[nodiscard]]
        bool empty() const
        {
            return safe_queue.empty();
        }

      private:
        tbb::concurrent_queue<Command> safe_queue;
    };
} // namespace Corona
