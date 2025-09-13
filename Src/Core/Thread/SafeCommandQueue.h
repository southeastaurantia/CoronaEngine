#pragma once
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
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
         * @brief 基于 std::invoke 的便利方法。
         * 接受一个可调用对象（如函数、函数对象、成员函数指针）及其参数，
         * 将它们封装为零参命令并入队，编译期校验可调用性。
         */
        template <typename F, typename... Args>
            requires std::is_invocable_v<F, Args...>
        void enqueue(F &&f, Args &&...args)
        {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            Command cmd = [fn = std::forward<F>(f), args = std::move(argsTuple)]() mutable {
                std::apply(
                    [&](auto &&...unpacked) {
                        std::invoke(fn, std::forward<decltype(unpacked)>(unpacked)...);
                    },
                    args);
            };
            safe_queue.push(std::move(cmd));
        }

        /**
         * @brief 对象实例 + 成员函数指针 的便捷入队（基于 std::invoke）。
         *
         * 使用示例：
         *   struct Foo { void bar(); void baz(int) const; };
         *   Foo a; auto *p = &a; auto sp = std::make_shared<Foo>();
         *   q.enqueue(a,  &Foo::bar);          // 复制/移动 a，随后调用 a.bar()
         *   q.enqueue(p,  &Foo::bar);          // 捕获指针，随后调用 p->bar()
         *   q.enqueue(std::ref(a), &Foo::bar); // 捕获引用包装，不复制对象（注意生命周期）
         *   q.enqueue(sp.get(), &Foo::baz, 42);// 使用智能指针时，传入 get() 或 std::ref(*sp)
         *
         * 约束：第二个参数必须是成员函数指针，且对 (obj, args...) 可调用，否则模板不参与重载选择。
         */
        template <typename Obj, typename MemFn, typename... CallArgs>
            requires(std::is_member_function_pointer_v<std::remove_reference_t<MemFn>> &&
                     std::is_invocable_v<MemFn, Obj, CallArgs...>)
        void enqueue(Obj &&obj, MemFn &&memfn, CallArgs &&...callArgs)
        {
            auto argsTuple = std::make_tuple(std::forward<CallArgs>(callArgs)...);
            Command cmd = [o = std::forward<Obj>(obj),
                           fn = std::forward<MemFn>(memfn),
                           args = std::move(argsTuple)]() mutable {
                std::apply(
                    [&](auto &&...unpacked) {
                        std::invoke(fn, o, std::forward<decltype(unpacked)>(unpacked)...);
                    },
                    args);
            };
            safe_queue.push(std::move(cmd));
        }

        /**
         * @brief 专门支持 std::shared_ptr<T> + 成员函数指针 的便捷入队：自动使用 sp.get() 调用。
         */
        template <typename T, typename MemFn, typename... CallArgs>
            requires(std::is_member_function_pointer_v<std::remove_reference_t<MemFn>> &&
                     std::is_invocable_v<MemFn, T *, CallArgs...>)
        void enqueue(std::shared_ptr<T> sp, MemFn &&memfn, CallArgs &&...callArgs)
        {
            auto argsTuple = std::make_tuple(std::forward<CallArgs>(callArgs)...);
            Command cmd = [sp = std::move(sp),
                           fn = std::forward<MemFn>(memfn),
                           args = std::move(argsTuple)]() mutable {
                std::apply(
                    [&](auto &&...unpacked) {
                        std::invoke(fn, sp.get(), std::forward<decltype(unpacked)>(unpacked)...);
                    },
                    args);
            };
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
