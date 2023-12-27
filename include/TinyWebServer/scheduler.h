#pragma once
#include <mutex>
#include "fiber.h"
#include <thread>
#include <list>
#include <atomic>
namespace WebSrv
{
    /**
     * @brief 协程调度器，内部包含一个线程池，n+m个协程，每个线程一个主协程和n个子协程
     *
     */
    class Scheduler
    {
    public:
        using ptr = std::shared_ptr<Scheduler>;
        /**
         * @brief Construct a new Scheduler object
         *
         * @param threads 线程数量
         * @param use_caller 是否由当前线程调用
         * @param name 协程调度器名
         */
        Scheduler(uint64_t threads = 1, bool use_caller = true, const std::string &name = "");

        virtual ~Scheduler();
        /**
         * @brief 返回协程调度器名
         *
         * @return const std::string&
         */
        const std::string &getName() const { return _name; }
        /**
         * @brief 返回当前协程调度器
         */
        static Scheduler *getThis();
        /**
         * @brief 主协程
         *
         * @return Fiber::ptr
         */
        static Fiber *getMainFiber();
        /**
         * @brief 开始运行
         *
         */
        void start();
        /**
         * @brief 停止运行
         *
         */
        virtual void stop();

        /**
         * @brief
         *
         * @tparam FiberOrCb
         * @param fc 协程或回调函数
         * @param thread 指定线程执行，-1为任意线程
         */
        template <class FiberOrCb>
        void schedule(FiberOrCb fc, std::thread::id threadid =std::thread::id())
        {
            bool needTickle = false;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                needTickle = scheduleNoLock(fc, threadid);
            }
            //当前无执行任务时立刻唤醒线程
            if (needTickle)
            {
                tickle();
            }
        }

        /**
         * @brief 批量添加协程或回调函数
         *
         * @tparam InputIterator
         * @param begin
         * @param end
         */
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                while (begin != end)
                {
                    need_tickle = scheduleNoLock(&*begin,std::thread::id()) || need_tickle;
                    ++begin;
                }
            }
            //当前无执行任务时立刻唤醒线程
            if (need_tickle)
            {
                tickle();
            }
        }
        /**
         * @brief 切换到指定线程执行
         *
         * @param thread
         */
        void switchTo(std::thread::id threadid = std::thread::id());

    protected:
        /**
         * @brief 通知调度器有任务发生
         * 
         */
        virtual void tickle();

        void run();

        virtual bool stopping();

        virtual void idle();

        void setThis();
        /**
         * @brief 是否有空闲协程
         * 
         */
        bool hasIdleThreads() { return _idleThreadCount > 0; }

    private:
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, std::thread::id threadid)
        {
            bool need_tickle = _fibers.empty();
            FiberAndThread ft(fc, threadid);
            if (ft._fiber || ft._cb)
            {
                _fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        /**
         * @brief 线程或协程
         *
         */
        struct FiberAndThread
        {
            Fiber::ptr _fiber;
            std::function<void()> _cb;
            std::thread::id _threadid;

            /**
             * @brief Construct a new Fiber And Thread object
             *
             * @param fiber
             * @param threadid
             */
            FiberAndThread(Fiber::ptr fiber,  std::thread::id threadid)
                : _fiber(fiber),
                  _threadid(threadid)
            {
            }

            /**
             * @brief Construct a new Fiber And Thread object
             *
             * @param fiber
             * @param threadid
             */
            FiberAndThread(Fiber::ptr *fiber,  std::thread::id threadid)
                : _threadid(threadid)
            {
                _fiber.swap(*fiber);
            }

            /**
             * @brief Construct a new Fiber And Thread object
             *
             * @param func
             * @param threadid
             */
            FiberAndThread(std::function<void()> func,  std::thread::id threadid)
                : _cb(func),
                  _threadid(threadid)
            {
            }

            /**
             * @brief Construct a new Fiber And Thread object
             *
             * @param func
             * @param threadid
             */
            FiberAndThread(std::function<void()> *func, std::thread::id threadid)
                : _threadid(threadid)
            {
                _cb.swap(*func);
            }

            FiberAndThread()
            {
            }

            /**
             * @brief 重置数据
             */
            void reset()
            {
                _fiber = nullptr;
                _cb = nullptr;
                _threadid= std::thread::id();
            }
        };

    private:
        std::mutex _mutex;
        std::string _name;
        /// @brief 线程池
        std::vector<std::thread> _threads;
        /// @brief 主线程的协程
        Fiber::ptr _rootFiber;
        /// @brief 协程任务队列
        std::list<FiberAndThread> _fibers;
    protected:
        /// @brief 协程下的线程id数组
        std::vector<std::thread::id> _threadIds;
        /// @brief 空闲协程数量
        std::atomic<uint64_t> _idleThreadCount = 0;
        /// @brief 活动协程数量
        std::atomic<uint64_t> _activeThreadCount = 0;
        /// @brief 是否正在停止
        bool _stopping = true;
        /// @brief 是否自动停止
        bool _autoStop = false;
        /// @brief 主线程id(use_caller)
        std::thread::id _rootThreadid;
        /// @brief 线程数量
        uint64_t _threadNum;
    };

} // namespace WebSrv
